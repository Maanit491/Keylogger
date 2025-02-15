<?php
$file = 'klog_main.exe'; // Change this to your actual file name

// Check if the file exists
if (!file_exists($file)) {
    die("Error: File not found.");
}

// Force download headers
header('Content-Description: File Transfer');
header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="' . basename($file) . '"');
header('Content-Length: ' . filesize($file));
header('Pragma: public');
header('Cache-Control: must-revalidate');
header('Expires: 0');

// Read the file
readfile($file);
exit;
?>
