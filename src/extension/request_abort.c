// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <SAPI.h>
#include <main/php_streams.h>
#include <php.h>
#include <stdio.h>
#include <stdlib.h>

#include "attributes.h"
#include "configuration.h"
#include "ddappsec.h"
#include "logging.h"
#include "php_compat.h"
#include "php_helpers.h"
#include "php_objects.h"
#include "request_abort.h"
#include "src/extension/string_helpers.h"

#define HTML_CONTENT_TYPE "text/html"
#define JSON_CONTENT_TYPE "application/json"

#define DEFAULT_RESPONSE_CODE 403
#define DEFAULT_RESPONSE_TYPE response_type_auto

static const char static_error_html[] =
    "<!-- Sorry, you’ve been blocked --><!DOCTYPE html><html lang=\"en\"><head>"
    "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-wid"
    "th,initial-scale=1\"><title>You've been blocked</title><style>a,body,div,h"
    "tml,span {margin: 0;padding: 0;border: 0;font-size: 100%;font: inherit;ver"
    "tical-align: baseline}body {background: -webkit-radial-gradient(26% 19%, c"
    "ircle, #fff, #f4f7f9);background: radial-gradient(circle at 26% 19%, #fff,"
    "#f4f7f9);display: -webkit-box;display: -ms-flexbox;display: flex;-webkit-b"
    "ox-pack: center;-ms-flex-pack: center;justify-content: center;-webkit-box-"
    "align: center;-ms-flex-align: center;align-items: center;-ms-flex-line-pac"
    "k: center;align-content: center;width: 100%;min-height: 100vh;line-height:"
    "1;flex-direction: column}p {display: block}main {text-align: center;flex: "
    "1;display: -webkit-box;display: -ms-flexbox;display: flex;-webkit-box-pack"
    ": center;-ms-flex-pack: center;justify-content: center;-webkit-box-align: "
    "center;-ms-flex-align: center;align-items: center;-ms-flex-line-pack: cent"
    "er;align-content: center;flex-direction: column}p {font-size: 18px;line-he"
    "ight: normal;color: #646464;font-family: sans-serif;font-weight: 400}a {co"
    "lor: #4842b7}footer {width: 100%;text-align: center}footer p {font-size: 1"
    "6px}</style></head><body><main><p>Sorry, you cannot access this page. Plea"
    "se contact the customer service team.</p></main><footer><p>Security provid"
    "ed by <ahref=\"https://www.datadoghq.com/product/security-platform/applica"
    "tion-security-monitoring/\"target=\"_blank\">Datadog</a></p></footer></bod"
    "y></html>";

static const char static_error_json[] =
    "{\"errors\": [{\"title\": \"You've been blocked\", \"detail\": \"Sorry, yo"
    "u cannot access this page. Please contact the customer service team. Secur"
    "ity provided by Datadog.\"}]}";

/*static zend_string *_custom_error_html = NULL;*/
/*static zend_string *_custom_error_json = NULL;*/
static THREAD_LOCAL_ON_ZTS int _response_code = DEFAULT_RESPONSE_CODE;
static THREAD_LOCAL_ON_ZTS dd_response_type _response_type =
    DEFAULT_RESPONSE_TYPE;

static void _abort_prelude(void);
ATTR_FORMAT(1, 2)
static void _emit_error(const char *format, ...);

zend_string *nullable read_file_contents(const char *nonnull path)
{
    php_stream *fs =
        php_stream_open_wrapper_ex(path, "rb", REPORT_ERRORS, NULL, NULL);
    if (fs == NULL) {
        return NULL;
    }

    zend_string *contents;
    contents = php_stream_copy_to_mem(fs, PHP_STREAM_COPY_ALL, 0);

    php_stream_close(fs);

    return contents;
}

static void _set_content_type(const char *nonnull content_type)
{
    char *ct_header; // NOLINT
    uint ct_header_len =
        (uint)spprintf(&ct_header, 0, "Content-type: %s", content_type);
    sapi_header_line line = {.line = ct_header, .line_len = ct_header_len};
    int res = sapi_header_op(SAPI_HEADER_REPLACE, &line);
    efree(ct_header);
    if (res == FAILURE) {
        mlog(dd_log_warning, "could not set content-type header");
    }
}

static void _set_output(const char *nonnull output, size_t length)
{
    size_t written = php_output_write(output, length);
    if (written != sizeof(static_error_html) - 1) {
        mlog(dd_log_info, "could not write full response (written: %zu)",
            written);
    }
}

static dd_response_type _get_response_type_from_accept_header()
{
    zval *_server =
        dd_php_get_autoglobal(TRACK_VARS_SERVER, LSTRARG("_SERVER"));
    if (!_server) {
        mlog(dd_log_info, "No SERVER autoglobal available");
        goto exit;
    }

    const zend_string *accept_zstr =
        dd_php_get_string_elem_cstr(_server, LSTRARG("HTTP_ACCEPT"));
    if (!accept_zstr) {
        mlog(dd_log_info, "Could not determine request URI");
        goto exit;
    }

    const char *accept_end = ZSTR_VAL(accept_zstr) + ZSTR_LEN(accept_zstr);
    const char *accept_json = memmem(ZSTR_VAL(accept_zstr),
        ZSTR_LEN(accept_zstr), LSTRARG(JSON_CONTENT_TYPE));
    const char *accept_json_end = accept_json + LSTRLEN(JSON_CONTENT_TYPE);

    if (accept_json != NULL && accept_json_end <= accept_end &&
        (*accept_json_end == ',' || *accept_json_end == '\0')) {
        return response_type_json;
    }

    const char *accept_html = memmem(ZSTR_VAL(accept_zstr),
        ZSTR_LEN(accept_zstr), LSTRARG(HTML_CONTENT_TYPE));
    const char *accept_html_end = accept_html + LSTRLEN(HTML_CONTENT_TYPE);

    if (accept_html != NULL && accept_html_end <= accept_end &&
        (*accept_html_end == ',' || *accept_html_end == '\0')) {
        return response_type_html;
    }

exit:
    return response_type_json;
}

void dd_request_abort_static_page()
{
    _abort_prelude();

    dd_response_type response_type = _response_type;
    if (response_type == response_type_auto) {
        response_type = _get_response_type_from_accept_header();
    }

    SG(sapi_headers).http_response_code = _response_code;
    if (response_type == response_type_html) {
        _set_content_type(HTML_CONTENT_TYPE);
        _set_output(static_error_html, sizeof(static_error_html) - 1);
    } else if (response_type == response_type_json) {
        _set_content_type(JSON_CONTENT_TYPE);
        _set_output(static_error_json, sizeof(static_error_json) - 1);
    } else {
        mlog(dd_log_warning, "unknown response type (bug) %d", response_type);
    }

    if (sapi_flush() != SUCCESS) {
        mlog(dd_log_info, "call to sapi_flush() failed");
    }

    _emit_error(
        "Datadog blocked the request and presented a static error page");
}

static void _force_destroy_output_handlers(void);
static void _abort_prelude()
{
    if (OG(running)) {
        /* we were told to block from inside an output handler. In this case,
         * we cannot use any output functions until we do some cleanup, as php
         * calls php_output_deactivate and issues an error in that case */
        _force_destroy_output_handlers();
    }

    if (SG(headers_sent)) {
        mlog(dd_log_info, "Headers already sent; response code was %d",
            SG(sapi_headers).http_response_code);
        _emit_error("Sqreen blocked the request, but the response has already "
                    "been partially committed");
        return;
    }

    int res = sapi_header_op(SAPI_HEADER_DELETE_ALL, NULL);
    if (res == SUCCESS) {
        mlog_g(dd_log_debug, "Cleared any current headers");
    } else {
        mlog_g(dd_log_warning, "Failed clearing current headers");
    }

    mlog_g(dd_log_debug, "Discarding output buffers");
    php_output_discard_all();
}
static void _force_destroy_output_handlers()
{
    OG(active) = NULL;
    OG(running) = NULL;

    if (OG(handlers).elements) {
        php_output_handler **handler;
        while ((handler = zend_stack_top(&OG(handlers)))) {
            php_output_handler_free(handler);
            zend_stack_del_top(&OG(handlers));
        }
    }
}

static void _run_rshutdowns(void);
static void _suppress_error_reporting(void);

ATTR_FORMAT(1, 2)
static void _emit_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (PG(during_request_startup)) {
        /* if emitting error during startup, RSHUTDOWN will not run (except fpm)
         * so we need to run the same logic from here */
        if (!get_global_DD_APPSEC_TESTING()) {
            mlog_g(
                dd_log_debug, "Running our RSHUTDOWN before aborting request");
            dd_appsec_rshutdown();
            DDAPPSEC_G(skip_rshutdown) = true;
        }
    }

    if ((PG(during_request_startup) &&
            strcmp(sapi_module.name, "fpm-fcgi") == 0)) {
        /* fpm children exit if we throw an error at this point. So emit only
         * warning and use other means to prevent the script from executing */
        php_verror(NULL, "", E_WARNING, format, args);
        // fpm doesn't try to run the script if it sees this null
        SG(request_info).request_method = NULL;
        return;
    }

    if (PG(during_request_startup)) {
        _run_rshutdowns();
    }

    /* Avoid logging the error message on error level. This is done by first
     * emitting it at E_COMPILE_WARNING level, supressing error reporting and
     * then re-emitting at error level, which does the bailout */

    /* hacky: use E_COMPILE_WARNING to avoid the possibility of it being handled
     * by a user error handler (as with E_WARNING). E_CORE_WARNING would also
     * be a possibility, but it bypasses the value of error_reporting and is
     * always logged */
    {
        va_list args2;
        va_copy(args2, args);
        php_verror(NULL, "", E_COMPILE_WARNING, format, args2);
        va_end(args2);
    }

    // not enough: EG(error_handling) = EH_SUPPRESS;
    _suppress_error_reporting();
    php_verror(NULL, "", E_ERROR, format, args);

    __builtin_unreachable();
    /* va_end(args); */ // never reached;
}

/* work around bugs in extensions that expect their request_shutdown to be
 * called once their request_init has been called */
static void _run_rshutdowns()
{
    HashPosition pos;
    zend_module_entry *module;
    bool found_ddappsec = false;

    mlog_g(dd_log_debug, "Running remaining extensions' RSHUTDOWN");
    for (zend_hash_internal_pointer_end_ex(&module_registry, &pos);
         (module = zend_hash_get_current_data_ptr_ex(&module_registry, &pos)) !=
         NULL;
         zend_hash_move_backwards_ex(&module_registry, &pos)) {
        if (!found_ddappsec && strcmp("ddappsec", module->name) == 0) {
            found_ddappsec = true;
            continue;
        }

        if (!module->request_shutdown_func) {
            continue;
        }

        if (found_ddappsec) {
            mlog_g(dd_log_debug, "Running RSHUTDOWN function for module %s",
                module->name);
            if (strcmp("ddtrace", module->name) == 0) {
                // DDTrace prevents flushing traces during RINIT, so we need to
                // trick into allowing it.
                PG(during_request_startup) = 0;
                module->request_shutdown_func(
                    module->type, module->module_number);
                PG(during_request_startup) = 1;
            } else {
                module->request_shutdown_func(
                    module->type, module->module_number);
            }
        }
    }
}

static void _suppress_error_reporting()
{
    /* do this through zend_alter_init_entry_ex rather than changing
     * EG(error_reporting) directly so the value is restored
     * on the deactivate phase (zend_ini_deactivate) */

    zend_string *name = zend_string_init(ZEND_STRL("error_reporting"), 0);
    zend_string *value = zend_string_init(ZEND_STRL("0"), 0);

    zend_alter_ini_entry_ex(
        name, value, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME, 1);

    zend_string_release(name);
    zend_string_release(value);
}

static PHP_FUNCTION(datadog_appsec_testing_abort_static_page)
{
    UNUSED(return_value);
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    dd_request_abort_static_page();
}

// clang-format off
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(no_params_void_ret, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry functions[] = {
    ZEND_RAW_FENTRY(DD_TESTING_NS "abort_static_page", PHP_FN(datadog_appsec_testing_abort_static_page), no_params_void_ret, 0)
    PHP_FE_END
};
// clang-format on

void dd_request_abort_startup()
{
    if (!get_global_DD_APPSEC_TESTING()) {
        return;
    }

    dd_phpobj_reg_funcs(functions);
}
