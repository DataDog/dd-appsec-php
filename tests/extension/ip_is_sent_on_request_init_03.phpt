--TEST--
Client ip is empty when dddtrace not available
--INI--
datadog.appsec.log_file=/tmp/php_appsec_test.log
datadog.appsec.log_level=debug
--ENV--
HTTP_X_FORWARDED_FOR=7.7.7.7
--FILE--
<?php
use function datadog\appsec\testing\{rinit,rshutdown};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([['ok']]);

var_dump(rinit());
var_dump(rshutdown());

$commands = $helper->get_commands();
$request_init = $commands[1];
echo $request_init[0].PHP_EOL;
var_dump($request_init[1][0]['server.request.ip']);
?>
--EXPECTF--
bool(true)
bool(true)
request_init
array(0) {
}