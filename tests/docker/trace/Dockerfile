ARG PHP_MAJOR_VERSION
ARG PHP_MINOR_VERSION
FROM datadog/dd-trace-ci:php-$PHP_MAJOR_VERSION.$PHP_MINOR_VERSION_centos-6 as trace-build

RUN mkdir /trace
COPY . /trace/

RUN cd /trace && make clean BUILD_DIR=/ext-build && \
    make all BUILD_DIR=/ext-build ECHO_ARG="-e" CFLAGS="-std=gnu11 -O2 -g -Wall -Wextra -Werror"

FROM scratch
COPY --from=trace-build /ext-build/modules/ddtrace.so /opt/
