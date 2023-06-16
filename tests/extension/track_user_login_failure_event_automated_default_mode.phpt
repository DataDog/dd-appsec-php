--TEST--
Metadata is discarded on automated safe mode
--INI--
extension=ddtrace.so
--ENV--
DD_APPSEC_ENABLED=1
--FILE--
<?php
use function datadog\appsec\testing\root_span_get_meta;
use function datadog\appsec\track_user_login_failure_event;
include __DIR__ . '/inc/ddtrace_version.php';

ddtrace_version_at_least('0.79.0');

track_user_login_failure_event("1234", true, ['something' => 'discarded'], true);

echo "root_span_get_meta():\n";
print_r(root_span_get_meta());
?>
--EXPECTF--
root_span_get_meta():
Array
(
    [appsec.events.users.login.failure.usr.id] => 1234
    [appsec.events.users.login.failure.track] => true
    [manual.keep] => true
    [_dd.appsec.events.users.login.failure.auto.mode] => safe
    [appsec.events.users.login.failure.usr.exists] => true
)
