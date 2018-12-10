<?php
use MessagePack\Packer;
use MessagePack\PackOptions;
use MessagePack\Type\Binary;
use MessagePack\TypeTransformer\BinaryTransformer;
use MessagePack\TypeTransformer\MapTransformer;
use MessagePack\Type\Map;
//use Packe

require __DIR__.'/vendor/autoload.php';

$packer = new Packer(PackOptions::FORCE_ARR);
$packer->registerTransformer(new MapTransformer());
$packedArray = $packer->pack([1, 2, 3]);
$packedMap = $packer->pack(new Map([1, 2, 3]));
$packedHash = $packer->pack(new Map(['a' => 1, 'b' => 2, 'c' => 3]));
var_dump($packedHash);



// настройки подключения
$address = "127.0.0.1";
$service_port = 4545;

// время перезагрузки страницы в секундах
$refreshTimeout = 1;

// подключение
$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($socket === false) 
{
    echo "\r\n\nCouldn't create socket.<br /> : " . socket_strerror(socket_last_error()) . "<br />Reconnect...<br />";
    continue;	

} else {
    echo "\r\n\nSocket created.<br />";
}

echo "<br />Trying to connect '$address' by port '$service_port'...<br /><br />";

$result = socket_connect($socket, $address, $service_port);
if ($result === false) 
{
    echo "Couldn't connect socket .<br/>: ($result) " . socket_strerror(
            socket_last_error($socket)
        ) . "<br />Reconnect...<br />";
    continue;	
} 
else 
{
    echo "Socked connected<br />";
}


$msg = $packedHash;
$len = strlen($msg);

//Отправка данных в сокет
if (socket_send($socket, $msg, $len, 0)) {
    echo "<br />" . "data successfully sent!<br /><br />";
}
echo "\r\nRESPONSE:\r\n";
echo "<pre><code>\r\n";
$out = socket_read($socket, 20);
echo htmlentities($out);

echo "</code></pre>";

socket_close($socket);
?>

<html>
<head>
   <!--	 <meta http-equiv="refresh" content="<?= $refreshTimeout ?>"/> -->
</head>
<body/>
</html>
