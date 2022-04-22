--TEST--
Verify ddappsec is always in the module registry after ddtrace
--SKIPIF--
<?php
if (strtoupper(PHP_OS) !== 'LINUX') {
    die('skip only for linux');
}
?>
--INI--
extension=ddtrace.so
zend_extension=opcache.so
--FILE--
<?php
print_r(get_loaded_extensions());
?>
--EXPECTREGEX--
.*
    \[\d+\] => ddtrace
    \[\d+\] => ddappsec
.*
