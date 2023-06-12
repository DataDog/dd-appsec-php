ARG PHP_VERSION
ARG VARIANT
FROM dd-appsec-php-extension:$PHP_VERSION-$VARIANT as extension
FROM composer:1.9.3 AS composer
FROM datadog/dd-appsec-php-ci:php-$PHP_VERSION-$VARIANT as ext-build

RUN apt-get update && apt-get install -y \
	apache2 \
    zip \
    sqlite3 \
	&& rm -rf /var/lib/apt/lists/*

COPY --from=composer /usr/bin/composer /usr/bin/composer

RUN rm -rf /var/www/html
ADD --chown=www-data:www-data examples/webroot/ /var/www/html/

COPY --from=extension /opt/ /opt/
RUN find /opt -name ddappsec.so -exec ln -s -v '{}' $(php -r 'echo ini_get("extension_dir");') \;

COPY --from=dd-appsec-php-helper /opt /opt
ADD tests/docker/php.ini /etc/php/
RUN PHP_INI_DIR=$(php-config --include-dir) && cp /etc/php/php.ini $PHP_INI_DIR && cp /etc/php/php.ini "/root/php/7.4.24-release/lib/"
ADD tests/docker/apache2-fpm/entrypoint.sh /

#Laravel 58
RUN cd /var/www/html/laravel58/ && php -d memory_limit=-1 /usr/bin/composer install --no-dev
RUN cd /var/www/html/laravel58/ && cp .env.example .env && php artisan key:generate
RUN cd /var/www/html/laravel58/ && touch /tmp/database58.sqlite && php artisan migrate && php artisan db:seed

#Laravel 8x
RUN cd /var/www/html/laravel8x/ && php -d memory_limit=-1 /usr/bin/composer install --no-dev
RUN cd /var/www/html/laravel8x/ && cp .env.example .env && php artisan key:generate
RUN cd /var/www/html/laravel8x/ && touch /tmp/database.sqlite && php artisan migrate && php artisan db:seed

ARG PHP_VERSION
ARG VARIANT
ADD tests/docker/apache2-fpm/php-site.conf /etc/apache2/sites-available/
ADD tests/docker/fpm-common/php-fpm.conf /etc/
RUN mkdir /etc/php-fpm.d/
ADD tests/docker/fpm-common/www.conf /etc/php-fpm.d/
RUN a2enmod proxy_fcgi
RUN a2dissite 000-default
RUN a2ensite php-site
RUN a2enmod rewrite

ENV DD_TRACE_ENABLED=true
ENV DD_TRACE_GENERATE_ROOT_SPAN=true

EXPOSE 80
ENTRYPOINT ["/entrypoint.sh"]

FROM ext-build as remote
ARG TRACER_VERSION
RUN curl -Lf -o /tmp/dd-library-php-setup.php https://raw.githubusercontent.com/DataDog/dd-appsec-php/installer/dd-library-php-setup.php && \
    PHP_INI_SCAN_DIR=/etc/php/ php /tmp/dd-library-php-setup.php --no-appsec --tracer-version $TRACER_VERSION --php-bin all

FROM dd-appsec-php-trace:$PHP_VERSION as trace
FROM ext-build as local

COPY --from=trace /opt/trace/bridge/ /opt/trace/bridge/
COPY --from=trace /opt/ddtrace.so /opt/

RUN find /opt -name ddtrace.so -exec ln -s -v '{}' $(php -r 'echo ini_get("extension_dir");') \;
