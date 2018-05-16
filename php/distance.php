<?php

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

$distance = getDistance(10, 100,0, 0);
echo "distance = $distance\n";