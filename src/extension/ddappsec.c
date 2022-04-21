// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <SAPI.h>
#include <Zend/zend_extensions.h>
#include <ext/standard/info.h>
#include <php.h>

// for open(2)
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdatomic.h>

#include "commands/client_init.h"
#include "commands/request_init.h"
#include "commands/request_shutdown.h"
#include "ddappsec.h"
#include "dddefs.h"
#include "ddtrace.h"
#include "helper_process.h"
#include "ip_extraction.h"
#include "logging.h"
#include "network.h"
#include "php_compat.h"
#include "php_helpers.h"
#include "php_objects.h"
#include "request_abort.h"
#include "string_helpers.h"
#include "tags.h"

#if ZTS
static atomic_int _thread_count;
#endif

static int _do_rinit(INIT_FUNC_ARGS);
static void _register_ini_entries(void);
#ifdef TESTING
static void _register_testing_objects(void);
#endif

static PHP_MINIT_FUNCTION(ddappsec);
static PHP_MSHUTDOWN_FUNCTION(ddappsec);
static PHP_RINIT_FUNCTION(ddappsec);
static PHP_RSHUTDOWN_FUNCTION(ddappsec);
static PHP_MINFO_FUNCTION(ddappsec);
static PHP_GINIT_FUNCTION(ddappsec);
static PHP_GSHUTDOWN_FUNCTION(ddappsec);
static int ddappsec_startup(zend_extension *extension);

ZEND_DECLARE_MODULE_GLOBALS(ddappsec)

// clang-format off
static const  zend_module_dep _ddappsec_deps[] = {
    ZEND_MOD_OPTIONAL("ddtrace")
    ZEND_MOD_END
};

static zend_module_entry ddappsec_module_entry = {
    STANDARD_MODULE_HEADER_EX,
    NULL,
    _ddappsec_deps,
    PHP_DDAPPSEC_EXTNAME,
    NULL,
    PHP_MINIT(ddappsec),
    PHP_MSHUTDOWN(ddappsec),
    PHP_RINIT(ddappsec),
    PHP_RSHUTDOWN(ddappsec),
    PHP_MINFO(ddappsec),
    PHP_DDAPPSEC_VERSION,
    PHP_MODULE_GLOBALS(ddappsec),
    PHP_GINIT(ddappsec),
    PHP_GSHUTDOWN(ddappsec),
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

static zend_extension ddappsec_extension_entry = {
    PHP_DDAPPSEC_EXTNAME,
    PHP_DDAPPSEC_VERSION,
    "Datadog",
    "https://github.com/DataDog/dd-appsec-php",
    "Copyright Datadog",
    ddappsec_startup,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    STANDARD_ZEND_EXTENSION_PROPERTIES};
// clang-format on

ZEND_GET_MODULE(ddappsec)

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void ddappsec_sort_modules(void *base, size_t count, size_t siz,
    compare_func_t compare, swap_func_t swp)
{
    UNUSED(siz);
    UNUSED(compare);
    UNUSED(swp);

    // Reorder ddappsec to ensure it's always after ddtrace
    for (Bucket *module = base, *end = module + count, *ddappsec_module = NULL;
         module < end; ++module) {
        zend_module_entry *m = (zend_module_entry *)Z_PTR(module->val);
        if (m->name == ddappsec_module_entry.name) {
            ddappsec_module = module;
            continue;
        }
        if (ddappsec_module && strcmp(m->name, "ddtrace") == 0) {
            Bucket tmp = *ddappsec_module;
            *ddappsec_module = *module;
            *module = tmp;
            break;
        }
    }
}

static int ddappsec_startup(zend_extension *extension)
{
    UNUSED(extension);

    zend_hash_sort_ex(&module_registry, ddappsec_sort_modules, NULL, 0);
    return SUCCESS;
}

// GINIT/GSHUTDOWN run before/after MINIT/MSHUTDOWN
static PHP_GINIT_FUNCTION(ddappsec)
{
#if defined(ZTS)
    TSRMLS_CACHE = tsrm_get_ls_cache();
#endif

#if ZTS
    atomic_fetch_add(&_thread_count, 1);
#endif

    memset(ddappsec_globals, '\0', sizeof(*ddappsec_globals)); // NOLINT
}

static PHP_GSHUTDOWN_FUNCTION(ddappsec)
{
    // delay log shutdown until the last possible moment, so that TSRM
    // destructors can run with logging
#if ZTS
    int prev = atomic_fetch_add(&_thread_count, -1);
    if (prev == 1) {
        dd_log_shutdown();
    }
#else
    dd_log_shutdown();
#endif

    memset(ddappsec_globals, '\0', sizeof(*ddappsec_globals)); // NOLINT
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static PHP_MINIT_FUNCTION(ddappsec)
{
    UNUSED(type);
    UNUSED(module_number);

    zend_register_extension(
        &ddappsec_extension_entry, ddappsec_module_entry.handle);

    dd_phpobj_startup(module_number);
    _register_ini_entries(); // depends on dd_phpobj_startup
    dd_log_startup();

#ifdef TESTING
    _register_testing_objects();
#endif

    dd_helper_startup();
    dd_trace_startup();
    dd_request_abort_startup();
    dd_tags_startup();
    dd_ip_extraction_startup();

    return SUCCESS;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static PHP_MSHUTDOWN_FUNCTION(ddappsec)
{
    UNUSED(type);
    UNUSED(module_number);

    dd_tags_shutdown();
    dd_trace_shutdown();
    dd_helper_shutdown();

    dd_phpobj_shutdown();

    return SUCCESS;
}

static PHP_RINIT_FUNCTION(ddappsec)
{
    if (!DDAPPSEC_G(enabled)) {
        mlog_g(dd_log_debug, "Appsec disabled");
        return SUCCESS;
    }

    DDAPPSEC_G(skip_rshutdown) = false;

    if (UNEXPECTED(DDAPPSEC_G(testing))) {
        if (DDAPPSEC_G(testing_abort_rinit)) {
            const char *pt = SG(request_info).path_translated;
            if (pt && !strstr(pt, "skip.php")) {
                dd_request_abort_static_page();
            }
        }
        return SUCCESS;
    }
    return _do_rinit(INIT_FUNC_ARGS_PASSTHRU);
}

static dd_result _acquire_conn_cb(dd_conn *nonnull conn)
{
    return dd_client_init(conn);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static int _do_rinit(INIT_FUNC_ARGS)
{
    UNUSED(type);
    UNUSED(module_number);

    dd_tags_rinit();

    // connect/client_init
    dd_conn *conn = dd_helper_mgr_acquire_conn(_acquire_conn_cb);
    if (conn == NULL) {
        mlog_g(dd_log_debug, "No connection; skipping rest of RINIT");
        return SUCCESS;
    }

    // request_init
    int res = dd_request_init(conn);
    if (res == dd_network) {
        mlog_g(dd_log_info, "request_init failed with dd_network; closing "
                            "connection to helper");
        dd_helper_close_conn();
    } else if (res == dd_should_block) {
        dd_request_abort_static_page();
    } else if (res) {
        mlog_g(
            dd_log_info, "request init failed: %s", dd_result_to_string(res));
    }

    return SUCCESS;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static PHP_RSHUTDOWN_FUNCTION(ddappsec)
{
    UNUSED(type);
    UNUSED(module_number);

    if (!DDAPPSEC_G(enabled)) {
        return SUCCESS;
    }

    if (DDAPPSEC_G(skip_rshutdown)) {
        return SUCCESS;
    }

    if (UNEXPECTED(DDAPPSEC_G(testing))) {
        dd_tags_rshutdown_testing();
        return SUCCESS;
    }

    return dd_appsec_rshutdown();
}

int dd_appsec_rshutdown()
{
    dd_conn *conn = dd_helper_mgr_cur_conn();
    if (conn) {
        // currently does nothing
        UNUSED(dd_request_shutdown(conn));
    }

    dd_helper_rshutdown();
    dd_tags_rshutdown();

    return SUCCESS;
}

static PHP_MINFO_FUNCTION(ddappsec)
{
    php_info_print_box_start(0);
    PUTS("Datadog PHP AppSec extension");
    PUTS(!sapi_module.phpinfo_as_text ? "<br>" : "\n");
    PUTS("(c) Datadog 2021\n");
    php_info_print_box_end();

    php_info_print_table_start();
    php_info_print_table_row(2, "Datadog AppSec support",
        DDAPPSEC_G(enabled) ? "enabled" : "disabled");
    php_info_print_table_row(2, "Version", PHP_DDAPPSEC_VERSION);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

#ifdef ZTS
__thread void *unspecnull TSRMLS_CACHE = NULL;
#endif

static ZEND_INI_MH(_on_update_appsec_enabled);
static ZEND_INI_MH(_on_update_appsec_enabled_on_cli);
static ZEND_INI_MH(_on_update_unsigned);

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

static void _register_ini_entries()
{
    // clang-format off
    static const dd_ini_setting settings[] = {
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
        {0}
    };
    // clang-format on

    dd_phpobj_reg_ini_envs(settings);
}

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

static PHP_FUNCTION(datadog_appsec_is_enabled)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
    RETURN_BOOL(DDAPPSEC_G(enabled));
}

static PHP_FUNCTION(datadog_appsec_testing_rinit)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    mlog(dd_log_debug, "Running rinit actions");
    int res = _do_rinit(MODULE_PERSISTENT, 0 /* we don't use it */);
    if (res == 0) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

static PHP_FUNCTION(datadog_appsec_testing_rshutdown)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    mlog(dd_log_debug, "Running rshutdown actions");
    int res = dd_appsec_rshutdown();
    if (res == 0) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
static PHP_FUNCTION(datadog_appsec_testing_helper_mgr_acquire_conn)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    dd_conn *conn = dd_helper_mgr_acquire_conn(_acquire_conn_cb);
    if (conn) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

static PHP_FUNCTION(datadog_appsec_testing_stop_for_debugger)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
    int fd = open(
        "/tmp/pid", O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0600); // NOLINT
    char pid[sizeof("-2147483648")] = "";
    sprintf(pid, "%" PRIi32, (int32_t)getpid()); // NOLINT
    ATTR_UNUSED ssize_t unused_ = write(fd, pid, strlen(pid));
    sleep(6); // NOLINT

    RETURN_TRUE;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(
    void_ret_bool_arginfo, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// clang-format off
static const zend_function_entry functions[] = {
    ZEND_RAW_FENTRY(DD_APPSEC_NS "is_enabled", PHP_FN(datadog_appsec_is_enabled), void_ret_bool_arginfo, 0)
    PHP_FE_END
};
static const zend_function_entry testing_functions[] = {
    ZEND_RAW_FENTRY(DD_TESTING_NS "rinit", PHP_FN(datadog_appsec_testing_rinit), void_ret_bool_arginfo, 0)
    ZEND_RAW_FENTRY(DD_TESTING_NS "rshutdown", PHP_FN(datadog_appsec_testing_rshutdown), void_ret_bool_arginfo, 0)
    ZEND_RAW_FENTRY(DD_TESTING_NS "helper_mgr_acquire_conn", PHP_FN(datadog_appsec_testing_helper_mgr_acquire_conn), void_ret_bool_arginfo, 0)
    ZEND_RAW_FENTRY(DD_TESTING_NS "stop_for_debugger", PHP_FN(datadog_appsec_testing_stop_for_debugger), void_ret_bool_arginfo, 0)
    PHP_FE_END
};
// clang-format on

static void _register_testing_objects()
{
    dd_phpobj_reg_funcs(functions);

    if (!DDAPPSEC_G(testing)) {
        return;
    }

    dd_phpobj_reg_funcs(testing_functions);
}
