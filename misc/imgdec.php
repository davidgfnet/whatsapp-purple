<?php

// Image decoder for WhatsApp encoded images!
// Use at your risk. If you set up a server with the script make me know about it!
// This has been borrowed from ChatAPI

// Check URL
if (!preg_match("/https:\/\/[a-z0-9]+\.whatsapp\.net/", $_GET["url"])) {
    die("URL not valid!\n");
}

function pkcs5_unpad($text)
{
    $pad = ord($text{strlen($text) - 1});
    if ($pad > strlen($text)) {
        return false;
    }
    if (strspn($text, chr($pad), strlen($text) - $pad) != $pad) {
        return false;
    }
    return substr($text, 0, -1 * $pad);
}

$file_enc = file_get_contents($_GET["url"]);
$cipherImage = substr($file_enc, 0, strlen($file_enc) - 10);

$key = hex2bin($_GET["key"]);
$iv  = hex2bin($_GET["iv"]);

$img = pkcs5_unpad(mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $key, $cipherImage, MCRYPT_MODE_CBC, $iv));

header("Content-Type: image/jpg");

echo $img;

?>
