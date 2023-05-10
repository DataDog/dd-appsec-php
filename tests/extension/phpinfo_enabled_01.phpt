--TEST--
Check enablement status by default
--FILE--
<?php
include __DIR__ . '/inc/phpinfo.php';

var_dump(get_configuration_value("Datadog AppSec status managed by remote config"));
var_dump(get_configuration_value("Datadog AppSec state"));

--EXPECT--
string(3) "Yes"
string(14) "Not configured"