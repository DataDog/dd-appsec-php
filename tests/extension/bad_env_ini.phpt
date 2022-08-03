--TEST--
Bad env var setting is ignored
--ENV--
DD_APPSEC_LOG_LEVEL=bad
--GET--
_force_cgi_sapi
--INI--
datadog.appsec.log_level=trace
--FILE--
<?php
// should be the hardcoded defaultl
var_dump(ini_get("datadog.appsec.log_level"));
?>
--EXPECTF--
string(5) "trace"
