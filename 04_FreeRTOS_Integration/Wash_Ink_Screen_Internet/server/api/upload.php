<?php
/**
 * upload.php — 上传 txt 书籍到书库
 *
 * POST multipart/form-data
 *   key:  API 密钥
 *   file: txt 文件
 *
 * 返回：{"status":"ok", "book": {...}}
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

define('API_KEY',    'ebook_upload_2026');
define('BOOKS_FILE', dirname(__DIR__) . '/usr/uploads/books.json');
define('UPLOAD_DIR', dirname(__DIR__) . '/usr/uploads');
define('BASE_URL',   'https://sprite-lover.top/usr/uploads');

/* ── 密钥验证 ── */
$key = $_POST['key'] ?? ($_SERVER['HTTP_X_API_KEY'] ?? '');
if ($key !== API_KEY) {
    http_response_code(403);
    echo json_encode(['status' => 'error', 'msg' => '密钥错误']);
    exit;
}

/* ── 检查文件 ── */
if (empty($_FILES['file']) || $_FILES['file']['error'] !== UPLOAD_ERR_OK) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'msg' => '未收到文件或上传出错']);
    exit;
}

$file    = $_FILES['file'];
$content = file_get_contents($file['tmp_name']);

/* 前端明确传了原始文件名则优先用，否则降级用 $_FILES 里的（微信临时路径名不可靠） */
$original_name = !empty($_POST['filename']) ? $_POST['filename'] : $file['name'];

/* ── 编码检测与转换 ── */
$original_encoding = 'GBK/ANSI';

if (substr($content, 0, 3) === "\xEF\xBB\xBF") {
    $content           = mb_convert_encoding(substr($content, 3), 'GBK', 'UTF-8');
    $original_encoding = 'UTF-8 with BOM';
} elseif (mb_check_encoding($content, 'UTF-8') && !mb_check_encoding($content, 'ASCII')) {
    $content           = mb_convert_encoding($content, 'GBK', 'UTF-8');
    $original_encoding = 'UTF-8';
}

/* ── 保存文件 ── */
$year     = date('Y');
$month    = date('m');
$dir      = UPLOAD_DIR . "/{$year}/{$month}";

if (!is_dir($dir) && !mkdir($dir, 0755, true)) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'msg' => '创建目录失败']);
    exit;
}

$filename = preg_replace('/[\/\\\:*?"<>|]/', '_', basename($original_name));
if (empty($filename) || $filename === '.txt') {
    $filename = 'book_' . date('YmdHis') . '.txt';
}

/* 文件名重复时加时间戳后缀 */
$savepath = "{$dir}/{$filename}";
if (file_exists($savepath)) {
    $name     = pathinfo($filename, PATHINFO_FILENAME);
    $ext      = pathinfo($filename, PATHINFO_EXTENSION);
    $filename = $name . '_' . date('His') . '.' . $ext;
    $savepath = "{$dir}/{$filename}";
}

if (file_put_contents($savepath, $content) === false) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'msg' => '写入文件失败']);
    exit;
}

$url = BASE_URL . "/{$year}/{$month}/{$filename}";

/* ── 追加到书库 ── */
$books = [];
if (file_exists(BOOKS_FILE)) {
    $books = json_decode(file_get_contents(BOOKS_FILE), true) ?? [];
}

$new_id = empty($books) ? 1 : (max(array_column($books, 'id')) + 1);

/* pathinfo() 在某些环境对中文文件名失效，用正则提取书名更可靠 */
$title = preg_replace('/\.[^.]+$/', '', $filename);
if (empty($title)) {
    $title = $filename;
}

$book = [
    'id'          => $new_id,
    'filename'    => $filename,
    'title'       => $title,
    'url'         => $url,
    'size_bytes'  => strlen($content),
    'encoding'    => $original_encoding,
    'uploaded_at' => date('Y-m-d H:i:s'),
];

$books[] = $book;
file_put_contents(BOOKS_FILE, json_encode($books, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT));

echo json_encode(['status' => 'ok', 'book' => $book], JSON_UNESCAPED_UNICODE);
