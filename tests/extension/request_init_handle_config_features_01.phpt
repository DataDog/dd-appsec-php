--TEST--
Request init handles getting disabled by config features
--INI--
datadog.appsec.enabled_on_cli=1
datadog.appsec.log_file=/tmp/php_appsec_test.log
--FILE--
<?php
use function datadog\appsec\testing\{rinit, rshutdown};

include __DIR__ . '/inc/mock_helper.php';
$helper = Helper::createInitedRun([
    response_list(response_config_features(false)),
]);

var_dump(\datadog\appsec\is_enabled());
rinit();
rshutdown();
var_dump(\datadog\appsec\is_enabled());

?>
--EXPECTF--
bool(true)
bool(false)
