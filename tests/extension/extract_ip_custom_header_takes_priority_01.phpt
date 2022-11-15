--TEST--
Client ip header takes priority over duplicated headers
--ENV--
HTTP_X_FORWARDED_FOR=7.7.7.7,10.0.0.1
HTTP_X_CLIENT_IP=7.7.7.7
HTTP_X_REAL_IP=7.7.7.8
HTTP_X_FORWARDED=for="foo"
HTTP_X_CLUSTER_CLIENT_IP=7.7.7.9
HTTP_FORWARDED_FOR=7.7.7.10,10.0.0.1
REMOTE_ADDR=7.7.7.12
HTTP_FOO_BAR=1.2.3.4
DD_TRACE_CLIENT_IP_HEADER=foo-Bar
--FILE--
<?php

use function datadog\appsec\testing\add_ancillary_tags;

$arr = array();
add_ancillary_tags($arr);
var_dump($arr['http.client_ip']);
var_dump(isset($arr['_dd.multiple-ip-headers']));
?>
--EXPECTF--
string(7) "1.2.3.4"
bool(false)
