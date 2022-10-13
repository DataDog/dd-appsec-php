--TEST--
ddappsec does not generate http.client_ip when DD_TRACE_CLIENT_IP_HEADER_DISABLED is true
--ENV--
HTTP_X_FORWARDED_FOR=7.7.7.7
DD_TRACE_CLIENT_IP_HEADER_DISABLED=true
--FILE--
<?php

use function datadog\appsec\testing\add_ancillary_tags;

$arr = array();
add_ancillary_tags($arr);

var_dump(isset($arr['http.client_ip']));
?>
--EXPECTF--
bool(false)
