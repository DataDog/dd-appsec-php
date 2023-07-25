package com.datadog.appsec.php.integration

import com.datadog.appsec.php.docker.AppSecContainer
import com.datadog.appsec.php.docker.FailOnUnmatchedTraces
import groovy.util.logging.Slf4j
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.condition.DisabledIf
import org.junit.jupiter.api.condition.EnabledIf
import org.testcontainers.junit.jupiter.Container
import org.testcontainers.junit.jupiter.Testcontainers

import static com.datadog.appsec.php.integration.TestParams.getPhpVersion
import static com.datadog.appsec.php.integration.TestParams.getTracerVersion
import static com.datadog.appsec.php.integration.TestParams.getVariant
import static org.testcontainers.containers.Container.ExecResult

@Testcontainers
@Slf4j
@DisabledIf('isZts')
class Apache2FpmTests implements CommonTests {
    static boolean zts = variant.contains('zts')
    static boolean laravel8Version = phpVersion.contains('7.4')
    static boolean symfony62Version = phpVersion.contains('8.1')
    @Container
    @FailOnUnmatchedTraces
    public static final AppSecContainer CONTAINER =
            new AppSecContainer(
                    imageDir: 'apache2-fpm',
                    phpVersion: phpVersion,
                    phpVariant: variant,
                    tracerVersion: tracerVersion
            )


    @Test
    void 'php-fpm -i does not launch helper'() {
        ExecResult res = CONTAINER.execInContainer('mkdir', '/tmp/cli/')

        res = CONTAINER.execInContainer(
                'bash', '-c',
                'php-fpm -d extension=ddtrace.so -d extension=ddappsec.so ' +
                        '-d datadog.appsec.enabled=0 ' +
                        '-d datadog.appsec.helper_runtime_path=/tmp/cli ' +
                        '-i')
        if (res.exitCode != 0) {
            throw new AssertionError("Failed executing php-fpm -i: $res.stderr")
        }
        res = CONTAINER.execInContainer('/bin/bash', '-c',
            'test $(find /tmp/cli/ -maxdepth 1 -type s -name \'ddappsec_*_*.sock\' | wc -l) -eq 0')
        assert res.exitCode == 0

        res = CONTAINER.execInContainer(
                'bash', '-c',
                'php-fpm -d extension=ddtrace.so -d extension=ddappsec.so ' +
                        '-d datadog.appsec.enabled=1 ' +
                        '-d datadog.appsec.helper_runtime_path=/tmp/cli ' +
                        '-i')
        if (res.exitCode != 0) {
            throw new AssertionError("Failed executing php-fpm -i: $res.stderr")
        }
        res = CONTAINER.execInContainer('/bin/bash', '-c',
            'test $(find /tmp/cli/ -maxdepth 1 -type s -name \'ddappsec_*_*.sock\' | wc -l) -eq 0')
        assert res.exitCode == 0
    }

    @Test
    void 'Pool environment'() {
        def trace = container.traceFromRequest('/plain/poolenv.php') { HttpURLConnection conn ->
            assert conn.responseCode == 200
            def content = conn.inputStream.text

            assert content.contains('Value of pool env is 10001')
        }
    }

    @Test
    @EnabledIf('isLaravel8Version')
    void 'Laravel 8x - login failure automated event'() {
      def trace = container.traceFromRequest('/laravel8x/public/authenticate?email=nonExisiting@email.com') { HttpURLConnection conn ->
                  assert conn.responseCode == 403
              }

      assert trace.meta."appsec.events.users.login.failure.track" == "true"
      assert trace.meta."_dd.appsec.events.users.login.failure.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.login.failure.usr.exists" == "false"
    }

    @Test
    @EnabledIf('isLaravel8Version')
    void 'Laravel 8x - login success automated event'() {
      //The user ciuser@example.com is already on the DB
      def trace = container.traceFromRequest('/laravel8x/public/authenticate?email=ciuser@example.com') { HttpURLConnection conn ->
                  assert conn.responseCode == 200
              }

      //ciuser@example.com user id is 1
      assert trace.meta."usr.id" == "1"
      assert trace.meta."_dd.appsec.events.users.login.success.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.login.success.track" == "true"
    }


    @Test
    @EnabledIf('isLaravel8Version')
    void 'Laravel 8x - sign up automated event'() {
      def trace = container.traceFromRequest(
      '/laravel8x/public/register?email=test-user-new@email.coms&name=somename&password=somepassword'
      ) { HttpURLConnection conn ->
                  assert conn.responseCode == 200
              }

      assert trace.meta."usr.id" == "2"
      assert trace.meta."_dd.appsec.events.users.signup.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.signup.track" == "true"
    }

    @Test
    @EnabledIf('isSymfony62Version')
    void 'Symfony 6 2 - login success automated event'() {
      //The user ciuser@example.com is already on the DB
      def trace = container.traceFromRequest('/symfony62/public/login', 'POST', '_username=test-user%40email.com&_password=test') { HttpURLConnection conn ->
                  assert conn.responseCode == 302
              }

     assert trace.meta."_dd.appsec.events.users.login.success.auto.mode" == "safe"
     assert trace.meta."appsec.events.users.login.success.track" == "true"
    }

    @Test
    @EnabledIf('isSymfony62Version')
    void 'Symfony 6 2 - login failure automated event'() {
      def trace = container.traceFromRequest('/symfony62/public/login', 'POST', '_username=aa&_password=ee') { HttpURLConnection conn ->
                  assert conn.responseCode == 302
              }
      assert trace.meta."appsec.events.users.login.failure.track" == 'true'
      assert trace.meta."_dd.appsec.events.users.login.failure.auto.mode" == 'safe'
      assert trace.meta."appsec.events.users.login.failure.usr.exists" == 'false'
    }
}
