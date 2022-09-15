--TEST--
when remote config returns enable the flow carry on
--FILE--
<?php
use function datadog\appsec\testing\{rinit,rshutdown};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([REMOTE_CONFIG_ENABLED, REQUEST_INIT_OK, shutdown_ok()]);

var_dump(rinit());

$commands = $helper->get_commands();

//If rc would say disabled, the third call would not happen
echo "The number of calls to helper are ".count($commands).PHP_EOL;

var_dump($commands[0][0]);
var_dump($commands[1][0]);
var_dump($commands[2][0]);

var_dump(rshutdown());
$commands = $helper->get_commands();

var_dump($commands[0][0]);

?>
--EXPECT--
bool(true)
The number of calls to helper are 3
string(11) "client_init"
string(13) "remote_config"
string(12) "request_init"
bool(true)
string(16) "request_shutdown"