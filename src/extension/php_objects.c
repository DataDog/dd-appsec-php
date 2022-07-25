// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <zend_API.h>
#include <zend_alloc.h>

#include "attributes.h"
#include "ddappsec.h"
#include "dddefs.h"
#include "php_compat.h"
#include "php_objects.h"

static int _module_number;
static zend_llist _function_entry_arrays;

static void _unregister_functions(void *zfe_arr_vp);

typedef struct _dd_registered_entries {
    char *nullable name;
    char *nullable env_name;
} dd_registered_entries;

#define DD_MAX_REGISTERED_ENTRIES 160
static uint8_t registered_entries_count = 0;
static dd_registered_entries registered_entries[DD_MAX_REGISTERED_ENTRIES];

void dd_phpobj_startup(int module_number)
{
    _module_number = module_number;
    zend_llist_init(&_function_entry_arrays,
        sizeof(const zend_function_entry *), _unregister_functions,
        1 /* persistent */);
}

dd_result dd_phpobj_reg_funcs(const zend_function_entry *entries)
{
    int res = zend_register_functions(NULL, entries, NULL, MODULE_PERSISTENT);
    if (res == FAILURE) {
        return dd_error;
    }
    zend_llist_add_element(&_function_entry_arrays, &entries);
    return dd_success;
}

dd_result dd_phpobj_reg_ini(const zend_ini_entry_def *entries)
{
    int res = zend_register_ini_entries(entries, _module_number);
    return res == FAILURE ? dd_error : dd_success;
}

#define NAME_PREFIX "datadog.appsec."
#define NAME_PREFIX_LEN (sizeof(NAME_PREFIX) - 1)
#define ENV_NAME_PREFIX "DD_APPSEC_"
#define ENV_NAME_PREFIX_LEN (sizeof(ENV_NAME_PREFIX) - 1)

#define ZEND_INI_MH_PASSTHRU entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage
static char *nullable _get_env_name_from_ini_name(
    const char *nullable name, size_t name_len);
static zend_string *nullable _fetch_from_env(const char *nullable env_name);
static ZEND_INI_MH(_on_modify_wrapper);
struct entry_ex {
    ZEND_INI_MH((*orig_on_modify));
    const char *hardcoded_def;
    uint16_t hardcoded_def_len;
    bool has_env;
    char _padding[5]; // NOLINT ensure padding is initialized to zeros
};
_Static_assert(sizeof(struct entry_ex) == 24, "Size is 24"); // NOLINT
_Static_assert(offsetof(zend_string, val) % _Alignof(struct entry_ex) == 0,
    "val offset of zend_string is compatible with alignment of entry_ex");
/*
 * This function gets call at minit, watch out with using Zend Memory Manager
 * No emallocs should be used. Instead pemallocs and similars.
 */
void dd_phpobj_reg_ini_env(const dd_ini_setting *sett)
{
    size_t name_len = NAME_PREFIX_LEN + sett->name_suff_len;
    char *name = safe_pemalloc(name_len, 1, ENV_NAME_PREFIX_LEN + 1, 1);
    memcpy(name, NAME_PREFIX, NAME_PREFIX_LEN);
    memcpy(name + NAME_PREFIX_LEN, sett->name_suff, sett->name_suff_len);
    name[name_len] = '\0';

    char *env_name =
        _get_env_name_from_ini_name(sett->name_suff, sett->name_suff_len);
    if (!env_name) {
        return;
    }
    zend_string *env_def = _fetch_from_env(env_name);

    zend_string *entry_ex_fake_str = zend_string_init_interned(
        (char *)&(struct entry_ex){
            .orig_on_modify = sett->on_modify,
            .hardcoded_def = sett->default_value,
            .hardcoded_def_len = sett->default_value_len,
            .has_env = env_def ? true : false,
        },
        sizeof(struct entry_ex), 1);

    const zend_ini_entry_def defs[] = {
        {
            .name = name,
            .name_length = (uint16_t)name_len,
            .modifiable = sett->modifiable,
            .value = env_def ? ZSTR_VAL(env_def) : sett->default_value,
            .value_length =
                env_def ? ZSTR_LEN(env_def) : (uint32_t)sett->default_value_len,
            .on_modify = _on_modify_wrapper,
            .mh_arg1 = (void *)(uintptr_t)sett->field_offset,
            .mh_arg2 = sett->global_variable,
            .mh_arg3 = ZSTR_VAL(entry_ex_fake_str),
        },
        {0}};

    if (dd_phpobj_reg_ini(defs) == dd_success &&
        registered_entries_count < DD_MAX_REGISTERED_ENTRIES) {
        registered_entries[registered_entries_count].name = name;
        registered_entries[registered_entries_count].env_name = env_name;
        registered_entries_count++;
    } else {
        if (name) {
            pefree(name, 1);
            name = NULL;
        }

        if (env_name) {
            pefree(env_name, 1);
            env_name = NULL;
        }
    }

    if (env_def) {
        zend_string_efree(env_def);
        env_def = NULL;
    }
}

/*
 * This function gets call at minit, watch out with using Zend Memory Manager
 * No emallocs should be used. Instead pemallocs and similars.
 */
static char *nullable _get_env_name_from_ini_name(
    const char *nullable name, size_t name_len)
{
    if (!name || name_len == 0) {
        return NULL;
    }

    size_t env_name_len = ENV_NAME_PREFIX_LEN + name_len;
    char *env_name = pemalloc(env_name_len + 1, 1);
    if (!env_name) {
        return NULL;
    }
    memcpy(env_name, ENV_NAME_PREFIX, ENV_NAME_PREFIX_LEN);

    const char *r = name;
    const char *rend = &name[name_len];
    char *w = &env_name[ENV_NAME_PREFIX_LEN];
    for (; r < rend; r++) {
        char c = *r;
        if (c >= 'a' && c <= 'z') {
            c -= 'a' - 'A';
        }
        *w++ = c;
    }
    *w = '\0';

    return env_name;
}

/**
 * This function is based on fpm_php_zend_ini_alter_master
 * from php-src
 */
static int _custom_php_zend_ini_alter_master(char *nullable name, size_t name_length,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    zend_string *nullable new_value, int mode, int stage,
    bool has_env) /* {{{ */
{
    zend_ini_entry *ini_entry;
    zend_string *duplicate;

    if (!name || name_length == 0 || !new_value) {
        return FAILURE;
    }

    if ((ini_entry = zend_hash_str_find_ptr(
             EG(ini_directives), name, name_length)) == NULL) {
        return FAILURE;
    }

    duplicate = zend_string_dup(new_value, 1);

    struct entry_ex *eex = ini_entry->mh_arg3;
    eex->has_env = has_env;

    if (!ini_entry->on_modify ||
        ini_entry->on_modify(ini_entry, duplicate, ini_entry->mh_arg1,
            ini_entry->mh_arg2, ini_entry->mh_arg3, stage) == SUCCESS) {
        ini_entry->value = duplicate;
        /* when mode == ZEND_INI_USER keep unchanged to allow ZEND_INI_PERDIR
         * (.user.ini) */
        if (mode == ZEND_INI_SYSTEM) {
            ini_entry->modifiable = mode;
        }
    } else {
        zend_string_free(duplicate);
    }

    return SUCCESS;
}

dd_result dd_phpobj_load_env_values()
{
    struct _dd_registered_entries current;
    zend_string *env_def;
    while (registered_entries_count > 0) {
        current = registered_entries[--registered_entries_count];
        env_def = _fetch_from_env(current.env_name);
        if (env_def) {
            _custom_php_zend_ini_alter_master(current.name,
                strlen(current.name), env_def, PHP_INI_SYSTEM,
                PHP_INI_STAGE_RUNTIME, true);
            zend_string_efree(env_def); // NOLINT
            env_def = NULL;
        }
        if (current.name) {
            pefree(current.name, 1);
            current.name = NULL;
        }
        if (current.env_name) {
            pefree(current.env_name, 1);
            current.env_name = NULL;
        }
    }
    return dd_success;
}

static zend_string *nullable _fetch_from_env(const char *nullable env_name)
{
    if (!env_name) {
        return NULL;
    }

    tsrm_env_lock();
    const char *res = getenv(env_name); // NOLINT
    tsrm_env_unlock();

    if (!res || *res == '\0') {
        return NULL;
    }
    return zend_string_init(res, strlen(res), 0);
}

static ZEND_INI_MH(_on_modify_wrapper)
{
    // env values have priority, except we still allow runtime overrides
    // this may be surprising, but it's what the tracer does

    struct entry_ex *eex = mh_arg3;

    if (!eex->has_env /* no env value, no limitations */ ||
        // runtime changes are still allowed
        stage != ZEND_INI_STAGE_STARTUP) {
        return eex->orig_on_modify(ZEND_INI_MH_PASSTHRU);
    }
    // else we have env value and we're at startup stage

    if (entry->value) {
        // if we have a value, we're either in the beginning of a new thread
        // or the value came from the the ini_def default (the env value)
        // in both cases we allow
        // see zend_register_ini_entries
        int res = eex->orig_on_modify(ZEND_INI_MH_PASSTHRU);
        if (UNEXPECTED(res == FAILURE)) {
            // if this fails though, we're in a bit of a problem. It means
            // that the env value is no good. We retry with the hardcoded
            // default, which should always work
            if (EXPECTED(entry->value)) {
                zend_string_release(entry->value);
            }
            entry->value = zend_string_init_interned(
                eex->hardcoded_def, eex->hardcoded_def_len, 1);
            new_value = entry->value; // modify argument variable
            res = eex->orig_on_modify(ZEND_INI_MH_PASSTHRU);
            UNUSED(res);
            assert(res == SUCCESS);
            return FAILURE;
        }
    }

    // else our env value was overriden by ini settings. we don't allow that
    // so we return FAILURE so that we run next with the ini_entry_def default,
    // i.e, the env value
    return FAILURE;
}

void dd_phpobj_reg_long_const(
    const char *name, size_t name_len, zend_long value, int flags)
{
    zend_register_long_constant(name, name_len, value, flags, _module_number);
}

void dd_phpobj_shutdown()
{
    zend_llist_destroy(&_function_entry_arrays);
    zend_unregister_ini_entries(_module_number);
}

static void _unregister_functions(void *zfe_arr_vp)
{
    const zend_function_entry **zfe_arr = zfe_arr_vp;
    int count = 0;
    for (const zend_function_entry *p = *zfe_arr; p->fname != NULL;
         p++, count++) {}
    zend_unregister_functions(*zfe_arr, count, NULL);
}
