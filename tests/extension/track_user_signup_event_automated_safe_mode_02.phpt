--TEST--
Safe mode does not allow sensitive ids
--INI--
extension=ddtrace.so
--ENV--
DD_APPSEC_ENABLED=1
DD_APPSEC_AUTOMATED_USER_EVENTS_TRACKING=safe
--FILE--
<?php
use function datadog\appsec\testing\root_span_get_meta;
use function datadog\appsec\track_user_signup_event;
include __DIR__ . '/inc/ddtrace_version.php';

ddtrace_version_at_least('0.79.0');

track_user_signup_event("sensitiveUserName", [], true);

echo "root_span_get_meta():\n";
print_r(root_span_get_meta());
?>
--EXPECTF--
root_span_get_meta():
Array
(
    [_dd.appsec.events.users.signup.auto.mode] => safe
    [appsec.events.users.signup.track] => true
)