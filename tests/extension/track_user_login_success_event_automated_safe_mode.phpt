--TEST--
Track automated user login success event with safe mode and verify the tags in the root span
--INI--
extension=ddtrace.so
--ENV--
DD_APPSEC_ENABLED=1
DD_APPSEC_AUTOMATED_USER_EVENTS_TRACKING=safe
--FILE--
<?php
use function datadog\appsec\testing\root_span_get_meta;
use function datadog\appsec\track_user_login_success_event;
include __DIR__ . '/inc/ddtrace_version.php';

ddtrace_version_at_least('0.79.0');

track_user_login_success_event("1234",
[
    "value" => "something",
    "metadata" => "some other metadata",
    "email" => "noneofyour@business.com"
],
true);

echo "root_span_get_meta():\n";
print_r(root_span_get_meta());
?>
--EXPECTF--
root_span_get_meta():
Array
(
    [usr.id] => 1234
    [manual.keep] => true
    [_dd.appsec.events.users.login.success.auto.mode] => safe
    [appsec.events.users.login.success.track] => true
    [appsec.events.users.login.success.value] => something
    [appsec.events.users.login.success.metadata] => some other metadata
    [appsec.events.users.login.success.email] => noneofyour@business.com
)