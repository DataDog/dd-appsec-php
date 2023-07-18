package com.datadog.appsec.php.integration

import com.datadog.appsec.php.docker.AppSecContainer
import com.datadog.appsec.php.docker.FailOnUnmatchedTraces
import groovy.util.logging.Slf4j
import org.junit.jupiter.api.condition.DisabledIf
import org.junit.jupiter.api.Test
import org.testcontainers.junit.jupiter.Container
import org.testcontainers.junit.jupiter.Testcontainers

import static com.datadog.appsec.php.integration.TestParams.getPhpVersion
import static com.datadog.appsec.php.integration.TestParams.getTracerVersion
import static com.datadog.appsec.php.integration.TestParams.getVariant
import static org.testcontainers.containers.Container.ExecResult

@Testcontainers
@Slf4j
class Apache2ModTests implements CommonTests {
    static boolean zts = variant.contains('zts')

    @Container
    @FailOnUnmatchedTraces
    public static final AppSecContainer CONTAINER =
            new AppSecContainer(
                    imageDir: 'apache2-mod',
                    phpVersion: phpVersion,
                    phpVariant: variant,
                    tracerVersion: tracerVersion
            )

    @Test
    @DisabledIf('isZts')
    void 'trace without attack after soft restart'() {
        ExecResult res = CONTAINER.execInContainer('service', 'apache2', 'reload')
        if (res.exitCode != 0) {
            throw new AssertionError("Failed reloading apache2: $res.stderr")
        }
        log.info "Result of restart: STDOUT: $res.stdout , STDERR: $res.stderr"

        def trace = CONTAINER.traceFromRequest('/plain/hello.php') {
            assert it.inputStream.text == 'Hello world!'
        }
        assert trace.metrics."_dd.appsec.enabled" == 1.0d
    }
}
