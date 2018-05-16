<?php

$crc_table = array (
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
        );


function doCRC (&$str, $pos, $length)
{
    $crc_table = $GLOBALS['crc_table'];
    $fcs = 0xFFFF;
    
    $end = $pos + $length;
    for ($i = $pos; $i < $end; $i++) {
        $fcs = $crc_table[(($fcs) ^ ord($str[$i])) & 0xFF] ^ ($fcs >> 8);
    }
        
    return (~$fcs) & 0xFFFF;
}

function checkCRC(&$str, $pos, $length)
{
    $crc_table = $GLOBALS['crc_table'];
    $fcs = 0xFFFF;
    
    $end = $pos + $length;
    for ($i = $pos; $i < $end; $i++) {
        $fcs = $crc_table[(($fcs) ^ ord($str[$i])) & 0xFF] ^ ($fcs >> 8);
    }
    
    return (($fcs & 0xFFFF) == 0xF0B8);
}

function printCurrentTime()
{
    date_default_timezone_set('PRC');
    $mtimestamp = sprintf("%.3f", microtime(true)); // 带毫秒的时间戳

    $timestamp = floor($mtimestamp); // 时间戳
    $milliseconds = round(($mtimestamp - $timestamp) * 1000); // 毫秒

    $datetime = date("Y-m-d H:i:s", $timestamp) . '.' . $milliseconds;
    echo sprintf("%s -> %s\n", $mtimestamp, $datetime);
}

function onMessage($server, $fd, $cmd, $content)
{
    $response = array();
    switch ($cmd)
    {
        case 0x01:  //register, device upload id to server.
            $fdinfo = $server->connection_info($fd);
            $response = onRegister($fd, $content, $fdinfo);
            break;
        case 0x02:  //GPS Location
            $response = onGPSLocation($fd, $content);
            break;
        case 0x03:  //device work status
            $response = onDeviceState($fd, $content);
            break;
        case 0x04:  //device output water status
            $response = onWaterState($fd, $content);
            break;
        case 0x05:  //ping
            $response = onPing($fd, $content);
            break;
        case 0x06:  //output water parameters. once output, total output.
            $response = onWaterResult($fd, $content);
            break;
        case 0xFF:  //terminal control test.
            onTest($server, $content);
            return;
    }
    
    if ($response[0]  == 1) {
        $cmd = $response[1];
        $content = $response[2];
        
        sendMessage($server, $fd, $cmd, $content);
        
    } else if ($response[0] < 0) {
        $server->close($fd);
    }
}

function onRegister($fd, $content, $fdinfo)
{
    if (strlen($content) != 35) {
        printf("invalid content length. need 35, but act:%d\n", strlen($content));
        return array(-1);
    }
    
    
    //var_dump($fdinfo);
    $imei = substr($content, 0, 15);
    $sim = substr($content, 15, 20);
    
    printf("imei: %s\n", $imei);
    printf("sim:  %s\n", $sim);
    
    global $imei_device_map;
    global $fd_imei_map;
    
    $info = new DeviceInfo();
    $exists = array_key_exists($imei, $imei_device_map);
    if ($exists) {
        $info = $imei_device_map[$imei];
        printf("found device for imei:%s\n", $imei);
    } else {
        printf("new device for imei:%s\n", $imei);
        $info->lng = 0;
        $info->lat = 0;
        $info->device_state = 0;
        $info->water_state = 0;
        $info->control_cmd = "";
    }
    
    $info->imei = $imei;
    $info->sim = $sim;
    $info->remote = $fdinfo["remote_ip"];
    $info->remote .= ":";
    $info->remote .= strval($fdinfo["remote_port"]);
    $info->fd = $fd;
    
    $imei_device_map[$imei] = $info;
    $fd_imei_map[$fd] = $imei;
    
    return array(0);
}

function onPing($fd, $content)
{
    $query = getDeviceInfo($fd);
    
    printf("recv ping\n");
    #var_dump($query);
    
    if ($query[0] == 0) {
        return array(1, 0x09, "");
    }
    return array(-1);
}

function convertGPS($latlng)
{
    $angle = intval(substr($latlng, 0, 2));
    $min   = intval(substr($latlng, 2, 2));
    $sec   = floatval(substr($latlng, 4));
    
    return $angle + ($min * 60 + $sec) / 3600;
}

function onGPSLocation($fd, $content)
{
    //11+1+13+1
    /*if (strlen($content) != 25) {
        printf("error, invalid length for gps protocol.%d\n", strlen($content));
        return array(-1);
    }
    printf("gps localtion\n");
    */
    
    $query = getDeviceInfo($fd);
    if ($query[0] < 0) {
        return array(-1);
    }
    
    $info = $query[1];
    $lng = floatval(substr($content, 0, 9));
    $lng = convertGPS($lng);
    if ($content[9] == "S") {  //south/north, south->nage
        $lng = -$lng;
    }
    $lat = floatval(substr($content, 10, 10));
    $lat = convertGPS($lat);
    if ($content[20] == "W") {  //west/east west->nagetive
        $lat = -$lat;
    }
   
    if (0 == $info->lng || 0 == $info->lat) {
        $info->lng = $lng;
        $info->lat = $lat;
        var_dump($info);
    } else if ($lng != $info->lng || $lat != $info->lat) {
        printf("location change (%f, %f) -> (%f, %f)\n", 
            $info->lng, $info->lat, $lng, $lat);
            
        $distance = getDistance($info->lng, $info->lat, $lng, $lat);
        printf("two gps distance:%f\n", $distance);        
    }
    
    return array(0, "");
}

function onDeviceState($fd, $content)
{
    if (strlen($content) == 0) {
        return array(-1);
    }
    
    printf("device state\n");
    
    $query = getDeviceInfo($fd);
    var_dump($query);
    if ($query[0] < 0) {
        return array(-1);
    }
    
    $info = $query[1];
    $state = ord($content[0]);
    
    switch ($state)
    {
    case 0x01:
        printf("device state: MAKE\n");
        break;
    case 0x02:
        printf("device state: CLEAN\n");
        break;
    case 0x03:
        printf("device state: STANDBY\n");
        break;
    case 0x04:
        printf("device state: LACK WATER\n");
        break;
    case 0x05:
        printf("device state: LOCK\n");
        break;
    default:
        printf("invalid device state:%02x\n", $state);
        return array(0);
    }
    $info->device_state = $state;
    
    return array(0, "");
}

function onWaterState($fd, $content)
{
    if (strlen($content) == 0) {
        return array(-1);
    }
    
    printf("water state\n");
    
    $query = getDeviceInfo($fd);
    if ($query[0] < 0) {
        return array(-1);
    }
    
    $info = $query[1];
    $state = ord($content[0]);
    
    switch ($state)
    {
    case 0x01:
        printf("water state: OUTPUT WATER\n");
        break;
    case 0x02:
        printf("water state: ALLOW WATER\n");
        break;
    case 0x03:
        printf("water state: SUPPLY WATER\n");
        break;
    case 0x04:
        printf("water state: BREAK DOWN\n");
        break;
    case 0x05:
        printf("water state: MACHINE LOCK\n");
        break;
    default:
        printf("invalid water state:%02x\n", $state);
        return array(0);
    }
    $info->water_state = $state;
    
    return array(0, "");
}

function onWaterResult($fd, $content)
{
    if (strlen($content) < 6) {
        return array(-1);
    }
    
    $once = ord($content[0]);
    $once = ($once << 8) | ord($content[1]);
    
    $total = ord($content[2]);
    $total = ($total << 8) | ord($content[3]);
    $total = ($total << 8) | ord($content[4]);
    $total = ($total << 8) | ord($content[5]);
    
    printf("out water:%d, total:%d\n", $once, $total);
    
    return array(0, "");
}

class DeviceInfo
{
    public $imei;
    public $sim;
    public $fd;
    #public $sockfd;
    public $remote;
    #public $connect_time;
    #public $last_time;
    public $lng;        //Longitude;
    public $lat;        //Latitude; 
    
    public $device_state;
    public $water_state;
    
    public $control_cmd;
}; 

function getDeviceInfo($fd)
{
    global $imei_device_map;
    global $fd_imei_map;
    
    $exists = array_key_exists($fd, $fd_imei_map);
    if (!$exists) {
        return array(-1, NULL);
    }
    
    $imei = $fd_imei_map[$fd];
    $exists = array_key_exists($imei, $imei_device_map);
    if (!$exists) {
        unset($fd_imei_map[$fd]);
        return array(-1, NULL);
    }
    
    return array(0, $imei_device_map[$imei]);
}

function onClose($fd)
{
    $query = getDeviceInfo($fd);
    
    if ($query[0] < 0) {
        printf("not found device for connection:%d\n", $fd);
        return;
    }
    
    global $fd_imei_map;
    unset($fd_imei_map[$fd]);
    
    $info = $query[1];
    $info->fd = 0;
    $info->remote = "";
    $info->control_cmd = "";
    
    global $imei_device_map;
    var_dump($imei_device_map);
}

function sendMessage($server, $fd, $cmd, $content)
{
    $len = 1/*head*/ + 1/*len*/ + 1/*cmd*/ + strlen($content) + 2/*crc*/ + 1/*tail*/;
    #printf("content:%d, len:%d\n", strlen($content), $len);
    
     //head
    $str .= chr(0xAA);  //head
    //msg size
    $str .= chr($len);
    //cmd
    $str .= chr($cmd);
    
    //content 
    $str .= $content;
    
    //crc
    $crc = doCRC($str, 1, strlen($str) - 1/*head*/);
    $str .= chr($crc & 0xFF);
    $str .= chr(($crc >> 8) & 0xFF);

    //tail
    $str .= chr(0xBB);
    
    printf("send message size:%d\n", strlen($str));
    for ($i = 0; $i < strlen($str); $i++) {
        printf("%02x ", ord($str[$i]));
    }
    printf("\n");

    //printCurrentTime();
    $server->send($fd, $str);
}

function doControl($server, $fd, $instruction, $enable, $params/*two bytes*/)
{
    $cmd = 0x06;
    $content = "";
    
    if ($instruction == "clean") {
        $content = chr(0x01);
    } else if ($instruction == "outwater") {
        $content = chr(0x02);
    } else if ($instruction == "lock") {
        $content = chr(0x03);
    } else {
        printf("unknown control instruction:%s\n", $instruction);
        return;
    }
    
    if ($enable) {
        $content .= chr(0x01);
    } else {
        $content .= chr(0x00);
    }
    $content .= chr($params & 0xFF);
    $content .= chr(($params >> 8) & 0xFF);
    
    sendMessage($server, $fd, $cmd, $content);
}

function doShow($server, $fd, $origin_tds, $prue_tds, $ph)
{
    $cmd = 0x08;
    
    $content .= chr($origin_tds & 0xFF);
    $content .= chr(($origin_tds >> 8) & 0xFF);
    
    $content .= chr($prue_tds & 0xFF);
    $content .= chr(($prue_tds >> 8) & 0xFF);
    
    $content .= chr($ph & 0xFF);
    $content .= chr(($ph >> 8) & 0xFF);
    
    sendMessage($server, $fd, $cmd, $content);
}

function doQuery($server, $fd, $instruction)
{
    $cmd = 0x07;
    $content = "";
    
    if ($instruction == "device") {
        $content = chr(0x01);
    } else if ($instruction == "id") {
        $content = chr(0x02);
    } else if ($instruction == "gps") {
        $content = chr(0x03);
    } else if ($instruction == "water") {
        $content = chr(0x04);
    } else {
        printf("unknown query instruction:%s\n", $instruction);
        return;
    }
    sendMessage($server, $fd, $cmd, $content);
}

function packageLengthFunc($data)
{
    $lstr = strlen($data);
    /*printf("length func size:%d\n", strlen($data));
    for ($i = 0; $i < strlen($data); $i ++) {
        printf("%02x ", ord($data[$i]));
    }
    printf("\n");*/
    
    if ($lstr < 6) {
        return 0;
    }

    $lproto = ord($data[1]);
    if ($lproto > $lstr) {
        return 0;
    }

    $head = ord($data[0]);
    $tail = ord($data[$lproto - 1]);
    if ($head != 0xAA || $tail != 0xBB) {
        echo "magic number invalid\n";
        return -1;
    }

    if (!checkCRC($data, 1, $lproto - 2)) {
        echo "crc not ok\n";
        return -1;
    }

    return $lproto;
}

function main()
{
    global $server;
    $server = new swoole_server("0.0.0.0", 9000);
    
    $server->set(array(
        #'daemonize' => true,
        'worker_num' => 1,
        'dispatch_mode' => 4,
        'backlog' => 128,
        'open_length_check' => true,
        'package_max_length' => 1024,
        #'package_length_type' => 'C',
        #'package_length_offset' => 1,
        #'package_body_offset' => 0,
        'package_length_func' => packageLengthFunc,
    ));
    
    $server->on('connect', function ($server, $fd){
        echo "connection open: {$fd}\n";
        printf("connnect PID:%d\n", getmypid());
    });
    
    $server->on('receive', function ($server, $fd, $reactor_id, $data) {
        //printCurrentTime();
        
        printf("recv fd:%d, size:%d\n", $fd, strlen($data));
        for ($i = 0; $i < strlen($data); $i ++) {
            printf("%02x ", ord($data[$i]));
        }
        printf("\n");
    
        $lproto = strlen($data);
        $cmd = ord($data[2]);
        $content = substr($data, 3, $lproto - 6);
        
        onMessage($server, $fd, $cmd, $content);
    });
    
    $server->on('close', function ($server, $fd) {
        echo "connection close: {$fd}\n";
        printf("close PID:%d\n", getmypid());
        
        onClose($fd);
    });
    
    $server->on('workerstart', function ($server, $worker_id){
        global $argv;
    
        /*echo "worker id:$worker_id, server:$server->worker_id, pid:$server->master_pid\n";
        echo "workder num:$server->setting['worker_num']\n";
        echo "worker: $server->taskworker\n\n\n";
        if($worker_id >= $server->setting['worker_num']) {
            swoole_set_process_name("php {$argv[0]} task worker");
        } else {
            #printf("worker PID:%d\n", getmypid());
            swoole_set_process_name("php {$argv[0]} event worker");
            #$client = new swoole_client(SWOOLE_SOCK_TCP);
        }*/
        
        /*swoole_timer_tick(15000, function ($timer_id) {
            echo "tick-15s, output water\n";
            global $server;
            global $imei_device_map;
            global $control_count;
            
            foreach ($imei_device_map as $value) { 
                $info = $value;
                
                $cmd = "outwater";
                $params = 520;
                $enable = false;
                
                if ($info->control_cmd == "") {
                    if ($control_count % 3 == 0) {
                        $cmd = "outwater";
                        $params = 520;
                    } else if ($control_count % 3 == 1) {
                        $cmd = "lock";
                        $params = 0;
                    } else {
                        $cmd = "clean";
                        $params = 100;
                    }
                    $enable = true;
                    $control_count = $control_count + 1;
                    $info->control_cmd = $cmd;
                } else {
                    $cmd    = $info->control_cmd;
                    $params = 0;
                    $enable = false;
                    $info->control_cmd = "";
                }
                
                if ($info->fd > 0) {
                    if ($cmd == "outwater") {
                        if ($info->water_state == 0x02) {
                            printf("device:[%s] can output water, send instruction.\n", $info->imei);
                            doControl($server, $info->fd, $cmd, $enable, $param);
                        } else if ($info->water_state == 0x01) {
                            printf("device:[%s] output water now, send instruction to stop.\n", $info->imei);
                            doControl($server, $info->fd, $cmd, $enable, $param);
                        }
                    } else if ($cmd == "clean") {
                        if ($info->water_state != 0x01 && $info->device_state != 0x02) {
                            printf("device:[%s] clean, send instruction.\n", $info->imei);
                            doControl($server, $info->fd, $cmd, $enable, $param);
                        } else if ($info->device_state == 0x02) {
                            printf("device:[%s] clean now, send instruction.\n", $info->imei);
                            doControl($server, $info->fd, $cmd, $enable, $param);
                        }
                    } else if ($cmd == "lock") {
                        printf("send lock machine to device[%s]->[%s]\n", $info->imei, $enable ? "lock" : "unlock");
                        doControl($server, $info->fd, $cmd, $enable, $param);
                    }
                } else {
                    printf("device:[%s] can not output water, water state(%d)\n", $info->imei,  $info->water_state);
                }
            } 
        });
        
        swoole_timer_tick(60 * 1000, function ($timer_id) {
            echo "tick-1min, send water paramters, tds, ph\n";
            global $server;
            global $imei_device_map;
            
            foreach ($imei_device_map as $value) { 
                $info = $value;
                
                if ($info->fd > 0) {  
                        
                    $origin_tds = rand(90, 110);
                    $prue_tds = rand(8, 13);
                    $ph = rand(670, 690);
                    printf("device:[%s] origin tds:%d, prue tds:%d, ph:%d\n", $info->imei, $origin_tds, $prue_tds, $ph);
                    
                    doShow($server, $info->fd, $origin_tds, $prue_tds, $ph);
                }
            } 
        });*/
        
        /*swoole_timer_tick(30 * 1000, function ($timer_id) {
            echo "tick-30000ms, query....\n";
            global $server;
            global $imei_device_map;
            
            foreach ($imei_device_map as $value) { 
                $info = $value;
                
                if ($info->fd > 0) {  
                    doQuery($server, $info->fd, "device");
                    doQuery($server, $info->fd, "id");
                    doQuery($server, $info->fd, "gps");
                    doQuery($server, $info->fd, "water");
                }
            } 
        });*/
        
        /*swoole_timer_tick(10000, function ($timer_id) {
            echo "tick-10000ms, clean machine\n";
            global $server;
            global $imei_device_map;
            
            foreach ($imei_device_map as $value) { 
                $info = $value;
                #$info = $imei_device_map[$key];
                if ($info->fd > 0 && $info->water_state == 0x02) {
                    printf("device:[%s] can output water, send instruction.\n", $info->imei);
                    #sendDeviceControlInstruction($server, $info->fd, 0x02);
                } else {
                    printf("device:[%s] can not output water, water state(%d)\n", $info->imei,  $info->water_state);
                }
            } 
        });*/
    });    
    
    $server->start();
}

$control_count = 0;
$server;
$imei_device_map = array(); 
$fd_imei_map = array();

main();


function onTest($server, $content)
{
    $imei = substr($content, 0, 15);
    $instruction = substr($content, 15);
    
    printf("imei:%s -> %s\n", $imei, $instruction);
    
    global $imei_device_map;
    $exists = array_key_exists($imei, $imei_device_map);
    if (!$exists) {
        printf("imei not infer device:%s\n", $imei);
        return;
    }
    
    $info = $imei_device_map[$imei];
    if ($info->fd == 0) {
        printf("device[%s] not connect\n", $imei);
        return;
    }
    
    if ($instruction == "show") {
        $origin_tds = rand(90, 110);
        $prue_tds = rand(8, 13);
        $ph = rand(670, 690);
        printf("device:[%s] origin tds:%d, prue tds:%d, ph:%d\n", $info->imei, $origin_tds, $prue_tds, $ph);
        
        doShow($server, $info->fd, $origin_tds, $prue_tds, $ph);
        
    } else if ($instruction == "startclean") {
        doControl($server, $info->fd, "clean", true, 30 * 60);  //clean on 4:00 AM, half hour.
    } else if ($instruction == "stopclean") {
        doControl($server, $info->fd, "clean", false, 0);
    } else if ($instruction == "startwater") {
        doControl($server, $info->fd, "outwater", true, 520);
    } else if ($instruction == "stopwater") {
        doControl($server, $info->fd, "outwater", false, 0);
    } else if ($instruction == "lock") {
        doControl($server, $info->fd, "lock", true, 0);
    } else if ($instruction == "unlock") {
        doControl($server, $info->fd, "lock", false, 0);
    } else if ($instruction == "querydevice") {
        doQuery($server, $info->fd, "device");
    } else if ($instruction == "queryid") {
        doQuery($server, $info->fd, "id");
    } else if ($instruction == "querygps") {
        doQuery($server, $info->fd, "gps");
    } else if ($instruction == "querywater") {
        doQuery($server, $info->fd, "water");
    }
}

function degree2Radians( $degree) 
{
    return $degree * pi() / 180.0;
}

function getDistance( $lat1,  $lng1,  $lat2,  $lng2)
{
    if (abs($lat1) > 90 || abs($lat2) > 90) {
        return -1;
    }
    
    if (abs($lng1) > 180 || abs($lng2) > 180) {
        return -1;
    }
    
    $rad_lat1 = degree2Radians($lat1);
    $rad_lat2 = degree2Radians($lat2);
    
    $a = $rad_lat1 - $rad_lat2;
    $b = degree2Radians($lng1) - degree2Radians($lng2);
    
    $t = sqrt(pow(sin($a / 2), 2) + cos($rad_lat1) * cos($rad_lat2) * pow(sin($b / 2), 2));
    $s = 2 * asin($t);
    $s = $s * 6378.137; //EARTH_RADIUS = 6378.137.

    return $s * 1000;   //convert km to meter.
}