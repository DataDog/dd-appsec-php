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
class Symfony62Tests {
    static boolean expectedVersion = phpVersion.contains('8.1') && !variant.contains('zts')

    AppSecContainer getContainer() {
            getClass().CONTAINER
    }

    @Container
    @FailOnUnmatchedTraces
    public static final AppSecContainer CONTAINER =
            new AppSecContainer(
                    imageDir: 'symfony62',
                    phpVersion: phpVersion,
                    phpVariant: variant,
                    tracerVersion: tracerVersion
            )

    @Test
    void 'Login success automated event'() {
      //The user ciuser@example.com is already on the DB
      def trace = container.traceFromRequest('/login', 'POST', '_username=test-user%40email.com&_password=test') { HttpURLConnection conn ->
                  assert conn.responseCode == 302
              }

     assert trace.meta."_dd.appsec.events.users.login.success.auto.mode" == "safe"
     assert trace.meta."appsec.events.users.login.success.track" == "true"
    }

    @Test
    void 'Login failure automated event'() {
      def trace = container.traceFromRequest('/login', 'POST', '_username=aa&_password=ee') { HttpURLConnection conn ->
                  assert conn.responseCode == 302
              }
      assert trace.meta."appsec.events.users.login.failure.track" == 'true'
      assert trace.meta."_dd.appsec.events.users.login.failure.auto.mode" == 'safe'
      assert trace.meta."appsec.events.users.login.failure.usr.exists" == 'false'
    }

    @Test
    void 'Sign up automated event'() {
      def trace = container.traceFromRequest(
      '/register', 'POST', 'registration_form[email]=some@email.com&registration_form[plainPassword]=somepassword&registration_form[agreeTerms]=1'
      ) { HttpURLConnection conn ->
                  assert conn.responseCode == 302
              }
      assert trace.meta."_dd.appsec.events.users.signup.auto.mode" == "safe"
      assert trace.meta."appsec.events.users.signup.track" == "true"
    }
}
