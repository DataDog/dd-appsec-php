--TEST--
Record response from request_init and ancillary tags in root span
--INI--
datadog.appsec.enabled=1
datadog.appsec.log_level=off
--FILE--
<?php
use function datadog\appsec\testing\rinit;

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_request_init(['block', ['type' => 'xml'], ['{"found":"attack"}','{"another":"attack"}']])),
], ['continuous' => true]);

rinit();

?>
--EXPECTHEADERS--
Status: 403 Forbidden
Content-type: application/json
--EXPECTF--
{"errors": [{"title": "You've been blocked", "detail": "Sorry, you cannot access this page. Please contact the customer service team. Security provided by Datadog."}]}
%s

Warning: datadog\appsec\testing\rinit(): Datadog blocked the request and presented a static error page in %s on line %d

%s
