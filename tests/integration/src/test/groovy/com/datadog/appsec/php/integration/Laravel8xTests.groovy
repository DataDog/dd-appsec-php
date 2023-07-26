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
@EnabledIf('isExpectedVersion')
class Laravel8xTests {
    static boolean expectedVersion = phpVersion.contains('7.4') && !variant.contains('zts')

    AppSecContainer getContainer() {
            getClass().CONTAINER
    }

    @Container
    @FailOnUnmatchedTraces
    public static final AppSecContainer CONTAINER =
            new AppSecContainer(
                    imageDir: 'laravel8x',
                    phpVersion: phpVersion,
                    phpVariant: variant,
                    tracerVersion: tracerVersion
            )

    @Test
    void 'Login failure automated event'() {
      def trace = container.traceFromRequest('/authenticate?email=nonExisiting@email.com', 'GET', null) { HttpURLConnection conn ->
                  assert conn.responseCode == 403
              }

      assert trace.meta."appsec.events.users.login.failure.track" == "true"
      assert trace.meta."_dd.appsec.events.users.login.failure.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.login.failure.usr.exists" == "false"
    }

    @Test
    void 'Login success automated event'() {
      //The user ciuser@example.com is already on the DB
      def trace = container.traceFromRequest('/authenticate?email=ciuser@example.com', 'GET', null) { HttpURLConnection conn ->
                  assert conn.responseCode == 200
              }

      //ciuser@example.com user id is 1
      assert trace.meta."usr.id" == "1"
      assert trace.meta."_dd.appsec.events.users.login.success.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.login.success.track" == "true"
    }


    @Test
    void 'Sign up automated event'() {
      def trace = container.traceFromRequest(
      '/register?email=test-user-new@email.coms&name=somename&password=somepassword', 'GET', null
      ) { HttpURLConnection conn ->
                  assert conn.responseCode == 200
              }

      assert trace.meta."usr.id" == "2"
      assert trace.meta."_dd.appsec.events.users.signup.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.signup.track" == "true"
    }
}
