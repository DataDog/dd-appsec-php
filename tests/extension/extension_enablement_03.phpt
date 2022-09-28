--TEST--
This test and siblings are in charge of testing all combinations of ini enablement and RC enablement
--DESCRIPTION--
INI not set
RC set to enabled
--FILE--
<?php
include __DIR__ . '/inc/mock_helper.php';

remote_config_set_enabled();
var_dump(\datadog\appsec\is_enabled());
?>
--EXPECTF--
bool(true)
