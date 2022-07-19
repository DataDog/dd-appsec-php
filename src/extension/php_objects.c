// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "php_objects.h"
#include "attributes.h"
#include "ddappsec.h"
#include "dddefs.h"
#include "php_compat.h"

static int _module_number;
static zend_llist _function_entry_arrays;

static void _unregister_functions(void *zfe_arr_vp);

typedef struct _dd_registered_entries {
    char *name;
    char *env_name;
} dd_registered_entries;

#define DD_MAX_REGISTERED_ENTRIES 160
uint8_t registered_entries_count = 0;
dd_registered_entries registered_entries[DD_MAX_REGISTERED_ENTRIES];

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
static char* _get_env_name_from_ini_name(const char *name, size_t name_len);
static zend_string *nullable _fetch_from_env(const char *env_name);
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

void dd_phpobj_reg_ini_env(const dd_ini_setting *sett)
{
    size_t name_len = NAME_PREFIX_LEN + sett->name_suff_len;
    char *name = malloc(name_len + 1);
    memcpy(name, NAME_PREFIX, NAME_PREFIX_LEN);
    memcpy(name + NAME_PREFIX_LEN, sett->name_suff, sett->name_suff_len);
    name[name_len] = '\0';

    zend_string *entry_ex_fake_str = zend_string_init_interned(
        (char *)&(struct entry_ex){
            .orig_on_modify = sett->on_modify,
            .hardcoded_def = sett->default_value,
            .hardcoded_def_len = sett->default_value_len,
            .has_env = false, //To review impact of hardcoding here
        },
        sizeof(struct entry_ex), 1);

    const zend_ini_entry_def defs[] = {
        {
            .name = name,
            .name_length = (uint16_t)name_len,
            .modifiable = sett->modifiable,
            .value = sett->default_value,
            .value_length = (uint32_t)sett->default_value_len,
            .on_modify = _on_modify_wrapper,
            .mh_arg1 = (void *)(uintptr_t)sett->field_offset,
            .mh_arg2 = sett->global_variable,
            .mh_arg3 = ZSTR_VAL(entry_ex_fake_str),
        },
        {0}};

    if (dd_phpobj_reg_ini(defs) == dd_success && registered_entries_count < DD_MAX_REGISTERED_ENTRIES) {
        registered_entries[registered_entries_count].name = name;
        registered_entries[registered_entries_count].env_name = _get_env_name_from_ini_name(sett->name_suff, sett->name_suff_len);
        registered_entries_count++;
    } else {
        free(name);
    }
}

static char* _get_env_name_from_ini_name(const char *name, size_t name_len)
{
    size_t env_name_len = ENV_NAME_PREFIX_LEN + name_len;
    char *env_name = malloc(env_name_len + 1);
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

int _alter_ini_entry_ex(zend_string *name, zend_string *new_value, int stage) /* {{{ */
{
    zend_ini_entry *ini_entry;
    zend_string *duplicate;
    zend_bool modified;

    if ((ini_entry = zend_hash_find_ptr(EG(ini_directives), name)) == NULL) {
        return FAILURE;
    }

    modified = ini_entry->modified;

    duplicate = zend_string_copy(new_value);

    if (!ini_entry->on_modify
        || ini_entry->on_modify(ini_entry, duplicate, ini_entry->mh_arg1, ini_entry->mh_arg2, ini_entry->mh_arg3, stage) == SUCCESS) {
        if (modified && ini_entry->orig_value != ini_entry->value) { /* we already changed the value, free the changed value */
            zend_string_release(ini_entry->value);
        }
        ini_entry->value = zend_string_dup(new_value, 1);
        ini_entry->orig_value = zend_string_dup(new_value, 1);
    } else {
        zend_string_release(duplicate);
        return FAILURE;
    }

    return SUCCESS;
}

dd_result dd_phpobj_load_env_values()
{
    struct _dd_registered_entries current;
    zend_string *zs_current_name = NULL;
    while (registered_entries_count > 0) {
        current = registered_entries[--registered_entries_count];
        zend_string *env_def = _fetch_from_env(registered_entries[registered_entries_count].env_name);
        if (env_def && ZSTR_LEN(env_def) > 0) {
            zs_current_name = zend_string_init(current.name, strlen(current.name), 0);
            _alter_ini_entry_ex(zs_current_name, env_def, PHP_INI_STAGE_RUNTIME);
            zend_string_efree(zs_current_name);
            zs_current_name = NULL;
        }
        if (current.name) {
            free(current.name);
            current.name = NULL;
        }
        if (env_def) {
            zend_string_efree(env_def);
            env_def = NULL;
        }
        if (current.env_name) {
            free(current.env_name);
            current.env_name = NULL;
        }
    }
    return dd_success;
}

static zend_string *nullable _fetch_from_env(const char *env_name)
{
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
