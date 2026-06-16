<?php
/**
 * books.php — 书库列表
 *
 * GET  /api/books.php          → 返回全部书籍列表
 * POST /api/books.php?action=delete&id=X  → 删除指定书籍（同时删文件）
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

define('API_KEY',    'ebook_upload_2026');
define('BOOKS_FILE', dirname(__DIR__) . '/usr/uploads/books.json');

$method = $_SERVER['REQUEST_METHOD'];
$action = $_GET['action'] ?? 'list';

if ($method === 'GET' && $action === 'list') {
    if (!file_exists(BOOKS_FILE)) {
        echo json_encode(['status' => 'ok', 'books' => []]);
        exit;
    }
    $books = json_decode(file_get_contents(BOOKS_FILE), true) ?? [];
    echo json_encode(['status' => 'ok', 'books' => $books], JSON_UNESCAPED_UNICODE);

} elseif ($action === 'delete') {
    /* 验证密钥 */
    $key = $_POST['key'] ?? ($_GET['key'] ?? '');
    if ($key !== API_KEY) {
        http_response_code(403);
        echo json_encode(['status' => 'error', 'msg' => '密钥错误']);
        exit;
    }

    $id    = (int)($_GET['id'] ?? 0);
    $books = file_exists(BOOKS_FILE)
           ? (json_decode(file_get_contents(BOOKS_FILE), true) ?? [])
           : [];

    $found = false;
    foreach ($books as $i => $b) {
        if ($b['id'] === $id) {
            /* 删除实体文件 */
            $path = dirname(__DIR__) . '/usr/uploads/' . parse_url($b['url'], PHP_URL_PATH);
            $path = str_replace('/usr/uploads/', '', parse_url($b['url'], PHP_URL_PATH));
            @unlink(dirname(__DIR__) . '/usr/uploads/' . ltrim($path, '/'));
            array_splice($books, $i, 1);
            $found = true;
            break;
        }
    }

    if (!$found) {
        http_response_code(404);
        echo json_encode(['status' => 'error', 'msg' => '书籍不存在']);
        exit;
    }

    file_put_contents(BOOKS_FILE, json_encode(array_values($books), JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT));
    echo json_encode(['status' => 'ok']);

} else {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'msg' => '未知操作']);
}
