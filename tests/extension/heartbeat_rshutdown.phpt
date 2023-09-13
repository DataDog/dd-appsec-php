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
use function datadog\appsec\testing\{rinit,rshutdown, root_span_get_metrics};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_request_init(['ok', [], []])),
    response_list(response_request_shutdown(['heartbeat', [], [], true, [], []])),
]);

var_dump(rinit());
var_dump(rshutdown());

echo "root_span_get_metrics():\n";
print_r(root_span_get_metrics());
?>
--EXPECTF--
bool(true)
bool(true)
root_span_get_metrics():
Array
(
    [%s] => %d
    [_dd.appsec.enabled] => 1
    [_dd.p.dm] => -5
    [_sampling_priority_v1] => 2
)
