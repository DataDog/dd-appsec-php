--TEST--
This test and siblings are in charge of testing all combinations of ini enablement and RC enablement
--DESCRIPTION--
INI set to enabled
RC not set
--INI--
datadog.appsec.enabled_on_cli=1
--FILE--
<?php
var_dump(\datadog\appsec\is_enabled());
?>
--EXPECT--
bool(true)
