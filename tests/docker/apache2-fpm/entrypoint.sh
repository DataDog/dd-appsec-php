#!/bin/bash -e


#LOGS_PHP=(/tmp/appsec.log /tmp/helper.log /tmp/php_error.log /tmp/php_fpm_error.log)
#touch "${LOGS_PHP[@]}"
#chown www-data:www-data "${LOGS_PHP[@]}"

LOGS_APACHE=(/var/log/apache2/{access.log,error.log})
touch "${LOGS_APACHE[@]}"
chown root:adm "${LOGS_APACHE[@]}"

env | sed 's/^/export /' >> /etc/apache2/envvars

php-fpm -y /etc/php-fpm.conf -c /etc/php/php.ini
service apache2 start

exec tail -f "${LOGS_PHP[@]}" "${LOGS_APACHE[@]}"
