ARG PHP_MAJOR_VERSION
ARG PHP_MINOR_VERSION
FROM datadog/dd-trace-ci:php-${PHP_MAJOR_VERSION}.${PHP_MINOR_VERSION}_centos-6 as trace-build

RUN mkdir /trace
COPY . /trace/

RUN cd /trace && make clean BUILD_DIR=/ext-build && \
    make all BUILD_DIR=/ext-build ECHO_ARG="-e" CFLAGS="-std=gnu11 -O2 -g -Wall -Wextra -Werror"

RUN curl -sS https://getcomposer.org/installer | php -- --install-dir=/usr/local/bin --filename=composer
RUN cd /trace && make generate

FROM scratch
COPY --from=trace-build /ext-build/modules/ddtrace.so /opt/
COPY --from=trace-build /trace/bridge/ /opt/trace/bridge/
