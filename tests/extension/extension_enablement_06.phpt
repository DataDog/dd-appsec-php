--TEST--
This test and siblings are in charge of testing all combinations of ini enablement and RC enablement
--DESCRIPTION--
INI set to disabled
RC set to disabled
--INI--
datadog.appsec.enabled_on_cli=0
--FILE--
<?php
include __DIR__ . '/inc/mock_helper.php';

remote_config_set_disabled();
var_dump(\datadog\appsec\is_enabled());
?>
--EXPECT--
bool(false)
