--TEST--
Ensure there are non required warnings when ddtrace is loaded but not enabled
--INI--
extension=ddtrace.so
datadog.appsec.log_file=/tmp/php_appsec_test.log
datadog.appsec.log_level=debug
datadog.appsec.enabled=1
--ENV--
DD_TRACE_ENABLED=false
--FILE--
<?php
use function datadog\appsec\testing\{rinit};
include __DIR__ . '/inc/ddtrace_version.php';
include __DIR__ . '/inc/logging.php';

ddtrace_version_at_least('0.79.0');

rinit();
not_in_log('/Expecting an object from \\\\ddtrace\\\\root_span/');

?>
--EXPECTF--
None of array (
  0 => '/Expecting an object from \\\\ddtrace\\\\root_span/',
) have matched