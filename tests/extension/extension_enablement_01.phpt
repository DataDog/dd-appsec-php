--TEST--
This test and siblings are in charge of testing all combinations of ini enablement and RC enablement
--DESCRIPTION--
INI not set
RC not set
--FILE--
<?php
var_dump(\datadog\appsec\is_enabled());
?>
--EXPECTF--
bool(false)
