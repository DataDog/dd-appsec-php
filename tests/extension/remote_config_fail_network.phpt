--TEST--
when remote config call fails the flow carry on as if extension not enabled
--FILE--
<?php
use function datadog\appsec\testing\{rinit,rshutdown,backoff_status,is_connected_to_helper};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([]); // respond to client_init but not remote_config

var_dump(rinit());

$commands = $helper->get_commands();

//If rc would say disabled, the second call would not happen
echo "The number of calls to helper are ".count($commands).PHP_EOL;

var_dump($commands[0][0]);

var_dump(rshutdown());
echo "is connected:\n";
var_dump(is_connected_to_helper());
var_dump(backoff_status());
?>
--EXPECTF--
Warning: datadog\appsec\testing\rinit(): [ddappsec] Error %s for command remote_config: dd_network in %s on line %d
bool(true)
The number of calls to helper are 1
string(11) "client_init"
bool(true)
is connected:
bool(false)
array(2) {
  ["failed_count"]=>
  int(1)
  ["next_retry"]=>
  float(%f)
}