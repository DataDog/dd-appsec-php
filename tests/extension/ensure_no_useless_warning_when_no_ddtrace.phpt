--TEST--
Ensure appsec is disabled when tracer is by env variables and no warning on the logs
--INI--
extension=ddtrace.so
datadog.appsec.testing=0
datadog.appsec.log_file=/tmp/php_appsec_test.log
datadog.appsec.log_level=warning
--ENV--
DD_TRACE_ENABLED=false
--FILE--
<?php
include __DIR__ . '/inc/ddtrace_version.php';
include __DIR__ . '/inc/logging.php';
ddtrace_version_at_least('0.79.0');

var_dump(\datadog\appsec\is_enabled());
not_in_log('/warning/');
?>
--EXPECTF--
bool(false)
None of array (
  0 => '/warning/',
) have matched