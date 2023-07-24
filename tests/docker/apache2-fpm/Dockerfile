ARG PHP_VERSION
ARG VARIANT
FROM dd-appsec-php-extension:$PHP_VERSION-$VARIANT as extension

FROM datadog/dd-appsec-php-ci:php-$PHP_VERSION-$VARIANT as ext-build
ARG PHP_VERSION

RUN apt-get update && apt-get install -y \
	apache2 \
	&& rm -rf /var/lib/apt/lists/*

ARG TRACER_VERSION
RUN curl -Lf -o /tmp/dd-library-php-setup.php https://raw.githubusercontent.com/DataDog/dd-appsec-php/installer/dd-library-php-setup.php && \
    PHP_INI_SCAN_DIR=/etc/php/ php /tmp/dd-library-php-setup.php --no-appsec --tracer-version $TRACER_VERSION --php-bin all


RUN rm -rf /var/www/html
ADD --chown=www-data:www-data examples/webroot/ /var/www/html/

COPY --from=extension /opt/ /opt/
RUN find /opt -name ddappsec.so -exec ln -s -v '{}' $(php -r 'echo ini_get("extension_dir");') \;

COPY --from=dd-appsec-php-helper /opt /opt
ADD tests/docker/php.ini /etc/php/
RUN echo "datadog.trace.request_init_hook=/opt/datadog/dd-library/$TRACER_VERSION/dd-trace-sources/bridge/dd_wrap_autoloader.php" >> /etc/php/php.ini
RUN cp /etc/php/php.ini $(php-config --ini-path)
ADD tests/docker/apache2-fpm/entrypoint.sh /

COPY --from=composer:latest /usr/bin/composer /usr/bin/composer
RUN if [ "$PHP_VERSION" = "7.4" ]; then \
    #Laravel 8x
    cd /var/www/html/laravel8x/ && php -d memory_limit=-1 /usr/bin/composer install --no-dev && \
    cp .env.example .env && php artisan key:generate && \
    touch /tmp/database.sqlite && chown www-data.www-data /tmp/database.sqlite && \
    php artisan migrate && php artisan db:seed; \
elif [ "$PHP_VERSION" = "8.1" ]; then \
    #Symfony 6.2
    cd /var/www/html/symfony62/ && php -d memory_limit=-1 /usr/bin/composer install; \
fi

RUN touch /tmp/appsec.log && chown www-data.www-data /tmp/appsec.log

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

