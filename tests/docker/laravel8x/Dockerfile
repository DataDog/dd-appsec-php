ARG PHP_VERSION
ARG VARIANT
ARG TRACER_VERSION

FROM dd-appsec-php-apache2-fpm:$PHP_VERSION-$VARIANT-tracer$TRACER_VERSION
ARG PHP_VERSION

RUN rm -rf /var/www/html
ADD --chown=www-data:www-data examples/laravel8x/ /var/www/html/

RUN rm /etc/apache2/sites-available/*
ADD tests/docker/laravel8x/php-site.conf /etc/apache2/sites-available/

COPY --from=composer:latest /usr/bin/composer /usr/bin/composer

#Laravel 8x
RUN cd /var/www/html/ && php -d memory_limit=-1 /usr/bin/composer install --no-dev && \
    cp .env.example .env && php artisan key:generate && \
    touch /tmp/database.sqlite && chown www-data.www-data /tmp/database.sqlite && \
    php artisan migrate && php artisan db:seed; \
