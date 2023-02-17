--TEST--
Test request execution. This php method will be removed once request_execution gets plugged into SDK
--INI--
extension=ddtrace.so
datadog.appsec.enabled=1
--FILE--
<?php
use function datadog\appsec\testing\{rinit,rshutdown,request_execution,request_execution_add_data};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_request_init(['ok', []])),
    response_list(response_request_init(['ok', []])), //This is dummy entry
    response_list(response_request_shutdown(['ok', [], new ArrayObject(), new ArrayObject()]))
]);

rinit();

///Ad data
$key01 = 'key 01';
$value01 = 'some value';
$key02 = 'key 02';
$value02 = 123;
$key03 = 'key 03';
$value03 = ['some' => 'array'];
request_execution_add_data($key01, $value01);
request_execution_add_data($key02, $value02);
request_execution_add_data($key03, $value03);

request_execution();
rshutdown();

$commands = $helper->get_commands();

var_dump($commands[2]);

?>
--EXPECTF--
array(2) {
  [0]=>
  string(17) "request_execution"
  [1]=>
  array(1) {
    [0]=>
    array(3) {
      ["key 01"]=>
      string(10) "some value"
      ["key 02"]=>
      int(123)
      ["key 03"]=>
      array(1) {
        ["some"]=>
        string(5) "array"
      }
    }
  }
}
