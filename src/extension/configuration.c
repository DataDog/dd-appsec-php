// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <SAPI.h>

#include "configuration.h"
#include "logging.h"
#include "php_objects.h"
#include "ip_extraction.h"
#include "ddappsec.h"

#define DEFAULT_OBFUSCATOR_KEY_REGEX                                           \
    "(?i)(?:p(?:ass)?w(?:or)?d|pass(?:_?phrase)?|secret|(?:api_?|private_?|"   \
    "public_?)key)|token|consumer_?(?:id|key|secret)|sign(?:ed|ature)|bearer|" \
    "authorization"

#define DEFAULT_OBFUSCATOR_VALUE_REGEX                                         \
    "(?i)(?:p(?:ass)?w(?:or)?d|pass(?:_?phrase)?|secret|(?:api_?|private_?|"   \
    "public_?|access_?|secret_?)key(?:_?id)?|token|consumer_?(?:id|key|"       \
    "secret)|sign(?:ed|ature)?|auth(?:entication|orization)?)(?:\\s*=[^;]|"    \
    "\"\\s*:\\s*\"[^\"]+\")|bearer\\s+[a-z0-9\\._\\-]+|token:[a-z0-9]{13}|gh[" \
    "opsu]_[0-9a-zA-Z]{36}|ey[I-L][\\w=-]+\\.ey[I-L][\\w=-]+(?:\\.[\\w.+\\/"   \
    "=-]+)?|[\\-]{5}BEGIN[a-z\\s]+PRIVATE\\sKEY[\\-]{5}[^\\-]+[\\-]{5}END[a-"  \
    "z\\s]+PRIVATE\\sKEY|ssh-rsa\\s*[a-z0-9\\/\\.+]{100,}"

static ZEND_INI_MH(_on_update_appsec_enabled);
static ZEND_INI_MH(_on_update_appsec_enabled_on_cli);
static ZEND_INI_MH(_on_update_unsigned);

static ZEND_INI_MH(_on_update_appsec_enabled)
{
    ZEND_INI_MH_UNUSED();
    // handle datadog.appsec.enabled
    bool is_cli =
        strcmp(sapi_module.name, "cli") == 0 || sapi_module.phpinfo_as_text;
    if (is_cli) {
        return SUCCESS;
    }

    bool ini_value = (bool)zend_ini_parse_bool(new_value);
    bool *val = &DDAPPSEC_NOCACHE_G(enabled);
    *val = ini_value;
    return SUCCESS;
}
static ZEND_INI_MH(_on_update_appsec_enabled_on_cli)
{
    ZEND_INI_MH_UNUSED();
    // handle datadog.appsec.enabled.cli
    bool is_cli =
        strcmp(sapi_module.name, "cli") == 0 || sapi_module.phpinfo_as_text;
    if (!is_cli) {
        return SUCCESS;
    }

    bool bvalue = (bool)zend_ini_parse_bool(new_value);
    DDAPPSEC_NOCACHE_G(enabled) = bvalue;
    return SUCCESS;
}

static ZEND_INI_MH(_on_update_unsigned)
{
    ZEND_INI_MH_UNUSED();

    char *endptr = NULL;
#define BASE 10
    long ini_value = strtol(ZSTR_VAL(new_value), &endptr, BASE);

    // If there is any error parsing, don't set the value.
    if (endptr == ZSTR_VAL(new_value) || *endptr != '\0') {
        return FAILURE;
    }

    if (ini_value < 0) {
        // If we have a negative value, assume the rate limit is disabled
        ini_value = 0;
    } else if (ini_value > UINT32_MAX) {
        // Limit the value to 32 bit max
        ini_value = UINT32_MAX;
    }

    unsigned *val = &DDAPPSEC_NOCACHE_G(trace_rate_limit);
    *val = ini_value;
    return SUCCESS;
}

// clang-format off
static const dd_ini_setting ini_settings[] = {
    DD_INI_ENV("enabled", "0", PHP_INI_SYSTEM, _on_update_appsec_enabled),
    DD_INI_ENV("enabled_on_cli", "0", PHP_INI_SYSTEM, _on_update_appsec_enabled_on_cli),
    DD_INI_ENV_GLOB("block", "0", PHP_INI_SYSTEM, OnUpdateBool, block, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("rules", "", PHP_INI_SYSTEM, OnUpdateString, rules_file, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("waf_timeout", "10000", PHP_INI_SYSTEM, OnUpdateLongGEZero, waf_timeout_us, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("trace_rate_limit", "100", PHP_INI_SYSTEM, _on_update_unsigned, trace_rate_limit, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("extra_headers", "", PHP_INI_SYSTEM, OnUpdateString, extra_headers, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("obfuscation_parameter_key_regexp", DEFAULT_OBFUSCATOR_KEY_REGEX, PHP_INI_SYSTEM, OnUpdateString, obfuscator_key_regex, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("obfuscation_parameter_value_regexp", DEFAULT_OBFUSCATOR_VALUE_REGEX, PHP_INI_SYSTEM, OnUpdateString, obfuscator_value_regex, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("testing", "0", PHP_INI_SYSTEM, OnUpdateBool, testing, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("testing_abort_rinit", "0", PHP_INI_SYSTEM, OnUpdateBool, testing_abort_rinit, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV_GLOB("testing_raw_body", "0", PHP_INI_SYSTEM, OnUpdateBool, testing_raw_body, zend_ddappsec_globals, ddappsec_globals),
    DD_INI_ENV("log_level", "warn", PHP_INI_ALL, on_update_log_level),
    DD_INI_ENV("log_file", "php_error_reporting", PHP_INI_SYSTEM, on_update_log_file),
    DD_INI_ENV("ipheader", "", PHP_INI_SYSTEM, on_update_ipheader),
    {0}
};
// clang-format on

void dd_configuration_startup(void)
{
    dd_phpobj_reg_ini_envs(ini_settings);
}

