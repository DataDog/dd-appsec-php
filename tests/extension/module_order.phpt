--TEST--
Verify ddappsec is always in the module registry after ddtrace
--INI--
extension=ddtrace.so
zend_extension=opcache.so
display_errors=Off
--FILE--
<?php
foreach (get_loaded_extensions() as &$ext) {
    if ($ext == 'ddappsec' || $ext == 'ddtrace') {
        printf("%s\n", $ext);
    }
}
?>
--EXPECTF--
ddtrace
ddappsec
