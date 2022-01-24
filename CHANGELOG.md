# Datadog AppSec for PHP Release

### v0.2.0
#### New Features
- (#2) Thread pool to reuse idle threads and reduce the client initialisation cost.
- (#3) Finalize daemon when idle for 24 hours.
- (#26) Add `DD_APPSEC_WAF_TIMEOUT` to allow configuration of the WAF timeout
- (#52) Validate daemon version on `client_init` to ensure compatibility
- (#53) Add client IP inferral from headers
- (#58) Socket and lock file versioning to avoid communication with incompatible daemons
- (#61) Basic AppSec trace rate limiting in the daemon, using `DD_APPSEC_TRACE_RATE_LIMIT`

#### Internal Changes
- (#1) Code scanning on daemon code.
- (#14) The list of response headers transmitted in the trace now contains only:
	- Content-type
	- Content-length
	- Content-encoding
	- Content-language
- (#18) Use clang-format and ensure copyright notices
- (#23) Integration tests for PHP 8.1
- (#33) Update standard logging to conform to the new specification
- (#37) Improve daemon test coverage
- (#44) Rename ini settings from `ddappsec.*` to `datadog.appsec.*`
- (#45) Add coverage report to helper
- (#54) Align ini and environment priorities with the tracer
- (#57) Override libfuzzer main to provide better control over object lifetimes
- (#60) Disable blocking by default
- (#60) Disable sending raw body to the daemon
- (#62) libddwaf upgraded to v1.0.17
- (#62) Updated ruleset to v1.2.4

#### Fixes
- (#15) Fix daemon launch on `php-fpm -i`
- (#55) Fix race condition when obtaining daemon UID/GID

### v0.1.0
- Initial release
