--TEST--
Tet request_execution
--INI--
datadog.appsec.enabled=1
--FILE--
<?php
use function datadog\appsec\testing\{request_execution};

include __DIR__ . '/inc/mock_helper.php';

$helper = Helper::createInitedRun([
    response_list(response_config_sync())
]);

request_execution();

$helper->print_commands();

?>
--EXPECTF--
Array
(
    [0] => Array
        (
            [0] => client_init
            [1] => Array
                (
                    [0] => %s
                    [1] => %s
                    [2] => %s
                    [3] => %d
                    [4] => Array
                        (
                            [app_version] => %s
                            [env] => 
                            [runtime_id] => 
                            [service] => appsec_tests
                            [tracer_version] => 0.84.0
                        )

                    [5] => Array
                        (
                            [obfuscator_key_regex] => hello
                            [obfuscator_value_regex] => goodbye
                            [rules_file] => /my/rules_file.json
                            [trace_rate_limit] => 100
                            [waf_timeout_us] => 42
                        )

                    [6] => Array
                        (
                            [enabled] => 1
                            [host] => 127.0.0.1
                            [max_payload_size] => 4096
                            [poll_interval] => 1000
                            [port] => 18126
                        )

                )

        )

    [1] => Array
        (
            [0] => request_execution
            [1] => Array
                (
                    [0] => Array
                        (
                            [http.client_ip] => 1.2.3.4
                            [server.request.body] => Array
                                (
                                )

                            [server.request.body.filenames] => Array
                                (
                                )

                            [server.request.body.files_field_names] => Array
                                (
                                )

                            [server.request.body.raw] => 
                            [server.request.cookies] => Array
                                (
                                    [c] => Array
                                        (
                                            [a] => 3
                                        )

                                    [d] => Array
                                        (
                                            [0] => 5
                                            [1] => 6
                                        )

                                )

                            [server.request.headers.no_cookies] => Array
                                (
                                    [content-length] => 0
                                    [content-type] => text/plain
                                )

                            [server.request.method] => GET
                            [server.request.path_params] => Array
                                (
                                    [0] => my
                                    [1] => ur+ i%0
                                )

                            [server.request.query] => Array
                                (
                                    [foo] => bar
                                    [key] => Array
                                        (
                                            [a] => Array
                                                (
                                                    [0] => val
                                                    [1] => val2
                                                )

                                        )

                                )

                            [server.request.uri.raw] => /my/ur+%00i%0/?key[a][]=v%61l&key[a][]=val2&foo=bar
                        )

                )

        )

)

