--TEST--
Verify configuration can be shared across dd extensions
--INI--
extension=ddtrace.so
--ENV--
DD_ENV=prod
DD_TRACE_CLIENT_IP_HEADER_DISABLED=true
--FILE--
<?php

use function datadog\appsec\testing\zai_config_get_value;
use function datadog\appsec\testing\zai_config_get_global_value;
$ddtrace_config = ini_get_all("ddtrace");

// DDtrace configuration is always expected to be owned by DDTrace due to
// enforced module order on zend activate

printf("DDTrace local configuration\n");
var_dump($ddtrace_config["datadog.service"]["local_value"]);
var_dump($ddtrace_config["datadog.env"]["local_value"]);
var_dump($ddtrace_config["datadog.trace.client_ip_header_disabled"]["local_value"]);

printf("\nDDAppSec local configuration\n");
var_dump(zai_config_get_value("DD_SERVICE"));
var_dump(zai_config_get_value("DD_ENV"));
var_dump(zai_config_get_value("DD_TRACE_CLIENT_IP_HEADER_DISABLED"));

printf("\nDDTrace global configuration\n");
var_dump($ddtrace_config["datadog.service"]["global_value"]);
var_dump($ddtrace_config["datadog.env"]["global_value"]);
var_dump($ddtrace_config["datadog.trace.client_ip_header_disabled"]["global_value"]);

printf("\nDDAppSec global configuration\n");
var_dump(zai_config_get_global_value("DD_SERVICE"));
var_dump(zai_config_get_global_value("DD_ENV"));
var_dump(zai_config_get_global_value("DD_TRACE_CLIENT_IP_HEADER_DISABLED"));

?>
--EXPECTF--
DDTrace local configuration
string(12) "appsec_tests"
string(4) "prod"
string(4) "true"

DDAppSec local configuration
string(12) "appsec_tests"
string(4) "prod"
bool(true)

DDTrace global configuration
string(12) "appsec_tests"
string(4) "prod"
string(4) "true"

DDAppSec global configuration
string(12) "appsec_tests"
string(4) "prod"
bool(true)
