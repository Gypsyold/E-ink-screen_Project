<?php
/**
 * device.php — 设备书架管理
 *
 * 微信小程序：
 *   GET  /api/device.php?action=list                          → 设备实际书籍列表
 *   GET  /api/device.php?action=add_delete&fn_gbk_hex=XXXX   → 请求删除某本书
 *
 * ESP32：
 *   POST /api/device.php?action=sync_files  body={"files":["hex1","hex2",...]}
 *                                                             → 上报 SD 卡实际文件列表
 *   GET  /api/device.php?action=get_delete                   → 取待删除任务
 *   GET  /api/device.php?action=done_delete&fn_gbk_hex=XXXX  → 标记删除完成
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST');
date_default_timezone_set('Asia/Shanghai');

define('DEVICE_FILE', dirname(__DIR__) . '/usr/uploads/device_state.json');

function load_dev() {
    return file_exists(DEVICE_FILE)
        ? (json_decode(file_get_contents(DEVICE_FILE), true) ?? [])
        : [];
}
function save_dev($data) {
    file_put_contents(DEVICE_FILE,
        json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT));
}

$g_lock = null;
function dev_lock() {
    global $g_lock;
    $g_lock = fopen(dirname(DEVICE_FILE) . '/device.lock', 'c');
    if ($g_lock) flock($g_lock, LOCK_EX);
}
function dev_unlock() {
    global $g_lock;
    if ($g_lock) { flock($g_lock, LOCK_UN); fclose($g_lock); $g_lock = null; }
}

/* GBK hex → UTF-8 书名（去掉 .txt 后缀）*/
function gbk_hex_to_title($hex) {
    $bytes = @hex2bin($hex);
    if ($bytes === false) return $hex;
    $utf8 = @mb_convert_encoding($bytes, 'UTF-8', 'GBK');
    if (!$utf8) $utf8 = $bytes;
    return preg_replace('/\.txt$/i', '', rtrim($utf8, "\0"));
}

$action = $_GET['action'] ?? 'list';

/* ══════════════════════════════════════════════════════════
 * POST ?action=sync_files  body: {"files":["hex1","hex2"]}
 * ESP32 上报 SD 卡 /ebooks/ 目录的实际文件列表
 * ══════════════════════════════════════════════════════════ */
if ($action === 'sync_files') {
    $body  = file_get_contents('php://input');
    $data  = json_decode($body, true);
    $hexes = $data['files'] ?? [];

    $books = [];
    foreach ($hexes as $hex) {
        if (!is_string($hex) || strlen($hex) === 0) continue;
        $books[] = [
            'fn_gbk_hex' => $hex,
            'title'      => gbk_hex_to_title($hex),
        ];
    }

    dev_lock();
    $dev                  = load_dev();
    $dev['device_files']  = $books;
    $dev['synced_at']     = date('Y-m-d H:i:s');
    save_dev($dev);
    dev_unlock();

    echo json_encode(['status' => 'ok', 'count' => count($books)], JSON_UNESCAPED_UNICODE);

/* ══════════════════════════════════════════════════════════
 * GET ?action=list → 设备书架（小程序）
 * 优先返回 ESP32 扫描的真实文件列表；
 * 若从未同步过则返回 "not_synced" 让小程序显示提示
 * ══════════════════════════════════════════════════════════ */
} elseif ($action === 'list') {
    $dev = load_dev();

    if (!empty($dev['device_files'])) {
        /* 过滤掉待删除列表中已经提交的文件 */
        $pending_hexes = array_column($dev['pending_deletes'] ?? [], 'fn_gbk_hex');
        $books = array_values(array_filter(
            $dev['device_files'],
            fn($b) => !in_array($b['fn_gbk_hex'], $pending_hexes)
        ));
        /* 按 GBK hex 字节序排序 = STM32 app_task.c 的 strcmp(fno.fname) 顺序 */
        usort($books, fn($a, $b) => strcmp($a['fn_gbk_hex'] ?? '', $b['fn_gbk_hex'] ?? ''));
        echo json_encode([
            'status'    => 'ok',
            'books'     => $books,
            'synced_at' => $dev['synced_at'] ?? '',
            'source'    => 'scan',
        ], JSON_UNESCAPED_UNICODE);
    } else {
        /* 尚未收到 ESP32 扫描数据 */
        echo json_encode([
            'status'  => 'ok',
            'books'   => [],
            'source'  => 'not_synced',
            'msg'     => '正在等待设备同步，请确保设备已联网',
        ], JSON_UNESCAPED_UNICODE);
    }

/* ══════════════════════════════════════════════════════════
 * GET ?action=add_delete&fn_gbk_hex=XXXX → 小程序请求删除
 * ══════════════════════════════════════════════════════════ */
} elseif ($action === 'add_delete') {
    $fn_hex = trim($_GET['fn_gbk_hex'] ?? '');
    if ($fn_hex === '') {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'msg' => '缺少 fn_gbk_hex']);
        exit;
    }

    dev_lock();
    $dev     = load_dev();
    $pending = $dev['pending_deletes'] ?? [];

    $already = false;
    foreach ($pending as $p) {
        if ($p['fn_gbk_hex'] === $fn_hex) { $already = true; break; }
    }
    if (!$already) {
        $pending[] = ['fn_gbk_hex' => $fn_hex, 'added_at' => date('Y-m-d H:i:s')];
        $dev['pending_deletes'] = $pending;
        save_dev($dev);
    }
    dev_unlock();
    echo json_encode(['status' => 'ok']);

/* ══════════════════════════════════════════════════════════
 * GET ?action=get_delete → ESP32 取待删除任务
 * ══════════════════════════════════════════════════════════ */
} elseif ($action === 'get_delete') {
    $dev     = load_dev();
    $pending = $dev['pending_deletes'] ?? [];
    if (empty($pending)) {
        echo json_encode(['status' => 'idle']);
        exit;
    }
    $task = $pending[0];
    echo json_encode([
        'status'     => 'pending',
        'fn_gbk_hex' => $task['fn_gbk_hex'],
    ]);

/* ══════════════════════════════════════════════════════════
 * GET ?action=done_delete&fn_gbk_hex=XXXX → ESP32 完成删除
 * ══════════════════════════════════════════════════════════ */
} elseif ($action === 'done_delete') {
    $fn_hex = trim($_GET['fn_gbk_hex'] ?? '');
    dev_lock();
    $dev     = load_dev();
    $pending = $dev['pending_deletes'] ?? [];
    $pending = array_values(array_filter($pending, fn($p) => $p['fn_gbk_hex'] !== $fn_hex));
    $dev['pending_deletes'] = $pending;
    /* 同步从 device_files 中也移除该文件 */
    $files = $dev['device_files'] ?? [];
    $files = array_values(array_filter($files, fn($b) => $b['fn_gbk_hex'] !== $fn_hex));
    $dev['device_files'] = $files;
    save_dev($dev);
    dev_unlock();
    echo json_encode(['status' => 'ok']);

} else {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'msg' => '未知 action']);
}
