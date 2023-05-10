--TEST--
Check enablement status when enabled by config
--INI--
datadog.appsec.enabled=1
--FILE--
<?php
include __DIR__ . '/inc/phpinfo.php';

var_dump(get_configuration_value("Datadog AppSec status managed by remote config"));
var_dump(get_configuration_value("Datadog AppSec state"));

--EXPECT--
string(2) "No"
string(7) "Enabled"