--TEST--
ddappsec generates http.client_ip when ddtrace does not
--ENV--
HTTP_X_FORWARDED_FOR=7.7.7.7
--FILE--
<?php
use function datadog\appsec\testing\add_ancillary_tags;


$arr = array();
add_ancillary_tags($arr);

var_dump($arr['http.client_ip']);
?>
--EXPECTF--
string(7) "7.7.7.7"
