<?php
/**
 * queue.php — 下载队列
 *
 * 微信小程序：
 *   POST /api/queue.php           body: {"key":"...","book_id":1}  → 加入队列
 *   GET  /api/queue.php?action=list                                 → 查看队列状态
 *
 * ESP32：
 *   GET  /api/queue.php?action=get                 → 取第一条 pending 任务
 *   GET  /api/queue.php?action=done&id=X           → 标记完成
 *   GET  /api/queue.php?action=progress&id=X&pct=Y → 上报进度
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
date_default_timezone_set('Asia/Shanghai');  /* 修复时间显示差8小时的问题 */

define('API_KEY',    'ebook_upload_2026');
define('BOOKS_FILE', dirname(__DIR__) . '/usr/uploads/books.json');
define('QUEUE_FILE', dirname(__DIR__) . '/usr/uploads/queue.json');

$method = $_SERVER['REQUEST_METHOD'];
$action = $_GET['action'] ?? ($method === 'POST' ? 'add' : 'get');

/* ── 加载辅助 ── */
function load_json($path) {
    return file_exists($path) ? (json_decode(file_get_contents($path), true) ?? []) : [];
}
function save_json($path, $data) {
    file_put_contents($path, json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT));
}

/* ── 文件锁（防止 ESP32 轮询与小程序取消并发写导致 JSON 读到空内容）── */
$g_lock_fp = null;
function queue_lock() {
    global $g_lock_fp;
    $lock = dirname(QUEUE_FILE) . '/queue.lock';
    $g_lock_fp = fopen($lock, 'c');
    if ($g_lock_fp) flock($g_lock_fp, LOCK_EX);
}
function queue_unlock() {
    global $g_lock_fp;
    if ($g_lock_fp) { flock($g_lock_fp, LOCK_UN); fclose($g_lock_fp); $g_lock_fp = null; }
}

/* ══════════════════════════════════════════
 * POST → 加入队列（微信小程序调用）
 * ══════════════════════════════════════════ */
if ($action === 'add') {
    $body = json_decode(file_get_contents('php://input'), true) ?? [];
    $key  = $body['key'] ?? ($_POST['key'] ?? '');

    if ($key !== API_KEY) {
        http_response_code(403);
        echo json_encode(['status' => 'error', 'msg' => '密钥错误']);
        exit;
    }

    $book_id = (int)($body['book_id'] ?? 0);
    if ($book_id <= 0) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'msg' => '缺少 book_id']);
        exit;
    }

    $books = load_json(BOOKS_FILE);
    $book  = null;
    foreach ($books as $b) {
        if ($b['id'] === $book_id) { $book = $b; break; }
    }
    if (!$book) {
        http_response_code(404);
        echo json_encode(['status' => 'error', 'msg' => '书籍不存在']);
        exit;
    }

    queue_lock();
    $queue  = load_json(QUEUE_FILE);
    $new_id = empty($queue) ? 1 : (max(array_column($queue, 'id')) + 1);

    $item = [
        'id'         => $new_id,
        'book_id'    => $book_id,
        'filename'   => $book['filename'],
        'title'      => $book['title'],
        'url'        => $book['url'],
        'size_bytes' => $book['size_bytes'] ?? 0,
        'status'     => 'pending',
        'progress'   => 0,
        'queued_at'  => date('Y-m-d H:i:s'),
    ];

    $queue[] = $item;
    save_json(QUEUE_FILE, $queue);
    queue_unlock();

    echo json_encode(['status' => 'ok', 'item' => $item], JSON_UNESCAPED_UNICODE);

/* ══════════════════════════════════════════
 * GET ?action=list → 队列状态（微信小程序）
 * ══════════════════════════════════════════ */
} elseif ($action === 'list') {
    $queue = load_json(QUEUE_FILE);
    $recent = array_slice(array_reverse($queue), 0, 20);
    echo json_encode(['status' => 'ok', 'queue' => $recent], JSON_UNESCAPED_UNICODE);

/* ══════════════════════════════════════════
 * GET ?action=get → ESP32 取待下载任务
 * ══════════════════════════════════════════ */
} elseif ($action === 'get') {
    queue_lock();
    $queue   = load_json(QUEUE_FILE);
    $now     = time();
    $timeout = 300;

    foreach ($queue as &$item) {
        $is_pending = ($item['status'] === 'pending');
        $is_stuck   = ($item['status'] === 'downloading')
                      && isset($item['started_at_unix'])
                      && ($now - $item['started_at_unix']) > $timeout;
        /* 兼容旧数据（无 started_at_unix 字段）*/
        if (!$is_stuck && $item['status'] === 'downloading' && isset($item['started_at']) && !isset($item['started_at_unix'])) {
            $is_stuck = ($now - strtotime($item['started_at'])) > $timeout;
        }

        if ($is_pending || $is_stuck) {
            /* 计算 GBK 编码的文件名（FatFS CODEPAGE=936 支持 GBK 中文），存入队列供删除时使用 */
            $fn_gbk_hex = bin2hex(mb_convert_encoding($item['filename'], 'GBK', 'UTF-8'));

            $item['status']          = 'downloading';
            $item['started_at']      = date('Y-m-d H:i:s');
            $item['started_at_unix'] = $now;           /* Unix 时间戳，供小程序 ETA 计算使用，无时区问题 */
            $item['device_fn_gbk']   = $fn_gbk_hex;   /* 设备上实际文件名（GBK hex），供删除时使用 */
            save_json(QUEUE_FILE, $queue);
            queue_unlock();

            echo json_encode([
                'status'       => 'pending',
                'id'           => $item['id'],
                'url'          => $item['url'],
                'filename'     => $item['filename'],
                'filename_gbk' => $fn_gbk_hex,   /* ESP32 用此字段向 STM32 发送中文文件名 */
            ], JSON_UNESCAPED_UNICODE);
            exit;
        }
    }

    queue_unlock();
    echo json_encode(['status' => 'idle']);

/* ══════════════════════════════════════════
 * GET ?action=cancel&id=X → 取消等待中/下载中的任务
 * ══════════════════════════════════════════ */
} elseif ($action === 'cancel') {
    $id = (int)($_GET['id'] ?? 0);
    queue_lock();
    $queue = load_json(QUEUE_FILE);
    $found = false;
    foreach ($queue as &$item) {
        if ((int)$item['id'] === $id && in_array($item['status'], ['pending', 'downloading'])) {
            $item['status']       = 'cancelled';
            $item['cancelled_at'] = date('Y-m-d H:i:s');
            $found = true;
            break;
        }
    }
    save_json(QUEUE_FILE, $queue);
    queue_unlock();
    echo json_encode(['status' => $found ? 'ok' : 'not_found']);

/* ══════════════════════════════════════════
 * GET ?action=progress&id=X&pct=Y → ESP32 上报下载进度
 * ══════════════════════════════════════════ */
} elseif ($action === 'progress') {
    $id  = (int)($_GET['id'] ?? 0);
    $pct = max(0, min(99, (int)($_GET['pct'] ?? 0)));
    queue_lock();
    $queue = load_json(QUEUE_FILE);
    foreach ($queue as &$item) {
        if ((int)$item['id'] === $id) {
            $item['progress'] = $pct;
            break;
        }
    }
    save_json(QUEUE_FILE, $queue);
    queue_unlock();
    echo json_encode(['status' => 'ok']);

/* ══════════════════════════════════════════
 * GET ?action=done&id=X → ESP32 完成上报
 * ══════════════════════════════════════════ */
} elseif ($action === 'done') {
    $id = (int)($_GET['id'] ?? 0);
    queue_lock();
    $queue = load_json(QUEUE_FILE);
    $found = false;
    foreach ($queue as &$item) {
        if ((int)$item['id'] === $id) {
            $item['status']   = 'done';
            $item['progress'] = 100;
            $item['done_at']  = date('Y-m-d H:i:s');
            $found = true;
            break;
        }
    }
    save_json(QUEUE_FILE, $queue);
    queue_unlock();
    echo json_encode(['status' => $found ? 'ok' : 'not_found']);

} else {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'msg' => '未知 action']);
}
