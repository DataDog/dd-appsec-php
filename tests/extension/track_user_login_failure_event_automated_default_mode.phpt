--TEST--
Track automated user login failure without specifying a mode event and verify the tags in the root span
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

track_user_login_failure_event("1234", true,
[
    "value" => "something",
    "metadata" => "some other metadata",
    "email" => "noneofyour@business.com"
], true);

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
    [appsec.events.users.login.failure.value] => something
    [appsec.events.users.login.failure.metadata] => some other metadata
    [appsec.events.users.login.failure.email] => noneofyour@business.com
)
