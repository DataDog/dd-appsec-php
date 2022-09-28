--TEST--
This test and siblings are in charge of testing all combinations of ini enablement and RC enablement
--DESCRIPTION--
INI set to enabled
RC set to enabled
--INI--
datadog.appsec.enabled_on_cli=1
--FILE--
<?php
include __DIR__ . '/inc/mock_helper.php';

remote_config_set_enabled();
var_dump(\datadog\appsec\is_enabled());
?>
--EXPECT--
bool(true)
