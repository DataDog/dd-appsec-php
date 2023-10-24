--TEST--
Schemas returned by helper on request shutdown are added to span
--INI--
extension=ddtrace.so
datadog.appsec.enabled=1
--FILE--
<?php
use function datadog\appsec\testing\{rinit, rshutdown, root_span_get_meta};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_request_init(['ok', [], []])),
    response_list(response_request_shutdown([
        'ok',
        [],
        [],
        true,
        [],
        [],
        ['schema_key_01' => 'schema_value 01', 'schema_key_02' => 'schema_value 02']
    ])),
]);

var_dump(rinit());
var_dump(rshutdown());

echo "root_span_get_meta():\n";
print_r(root_span_get_meta());

$helper->finished_with_commands();
?>
--EXPECTF--
bool(true)
bool(true)
root_span_get_meta():
Array
(
    [runtime-id] => %s
    [schema_key_01] => schema_value 01
    [schema_key_02] => schema_value 02
    [_dd.runtime_family] => php
)
