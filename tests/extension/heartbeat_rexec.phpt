--TEST--
When a heartbeat is generated, tags are set accordingly
--INI--
extension=ddtrace.so
datadog.appsec.enabled=1
datadog.appsec.log_file=/tmp/php_appsec_test.log
--ENV--
DD_APM_TRACING_ENABLED=false
--FILE--
<?php
use function datadog\appsec\testing\{rinit, rshutdown, request_exec, root_span_get_metrics};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_request_init(['ok', [], []])),
    response_list(response_request_exec(['heartbeat', [], [], true])),
]);

rinit();
request_exec([
    'key 01' => 'some value',
    'key 02' => 123,
    'key 03' => ['some' => 'array']
]);
rshutdown();

echo "root_span_get_metrics():\n";
print_r(root_span_get_metrics());
?>
--EXPECTF--
root_span_get_metrics():
Array
(
    [%s] => %d
    [_dd.appsec.enabled] => 1
    [_dd.p.dm] => -5
    [_sampling_priority_v1] => 2
)
