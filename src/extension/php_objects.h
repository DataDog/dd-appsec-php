// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "dddefs.h"
#include <zend_API.h>
#include <zend_globals.h>
#include <zend_ini.h>

#define DD_APPSEC_NS "datadog\\appsec\\"
#define DD_TESTING_NS "datadog\\appsec\\testing\\"

#define APPSEC_NAME_PREFIX "datadog.appsec."
#define APPSEC_NAME_PREFIX_LEN (sizeof(APPSEC_NAME_PREFIX) - 1)
#define APPSEC_ENV_NAME_PREFIX "DD_APPSEC_"
#define APPSEC_ENV_NAME_PREFIX_LEN (sizeof(APPSEC_ENV_NAME_PREFIX) - 1)

#define TRACE_NAME_PREFIX "datadog.trace."
#define TRACE_NAME_PREFIX_LEN (sizeof(TRACE_NAME_PREFIX) - 1)
#define TRACE_ENV_NAME_PREFIX "DD_TRACE_"
#define TRACE_ENV_NAME_PREFIX_LEN (sizeof(TRACE_ENV_NAME_PREFIX) - 1)

typedef struct _dd_ini_setting {
    const char *name_suff; // the part after 'ddappsec'
    const char *default_value;
    uint16_t name_suff_len;
    uint16_t default_value_len;
    int modifiable;
    ZEND_INI_MH((*on_modify));

    /* optional */
    // either a pointer to the read global variable,
    // or a pointer to int representing the tsrm offset
    void *global_variable;
    size_t field_offset;
    const char *name_prefix;
    uint16_t name_prefix_len;
    const char *env_name_prefix;
    uint16_t env_name_prefix_len;
    int avoid_registering_twice;
} dd_ini_setting;

#define DD_INI_ENV(name_suff, default_value, modifiable, on_modify,            \
    name_prefix, name_prefix_len, env_name, env_name_len,                      \
    avoid_registering_twice)                                                   \
    ((dd_ini_setting){(name_suff), (default_value), sizeof(name_suff "") - 1,  \
        sizeof(default_value "") - 1, (modifiable), (on_modify), NULL, 0,      \
        name_prefix, name_prefix_len, env_name, env_name_len,                  \
        avoid_registering_twice})

#define DD_APPSEC_INI_ENV(name_suff, default_value, modifiable, on_modify)     \
    DD_INI_ENV(name_suff, default_value, modifiable, on_modify,                \
        APPSEC_NAME_PREFIX, APPSEC_NAME_PREFIX_LEN, APPSEC_ENV_NAME_PREFIX,    \
        APPSEC_ENV_NAME_PREFIX_LEN, 0)

#define DD_TRACE_INI_ENV(name_suff, default_value, modifiable, on_modify)      \
    DD_INI_ENV(name_suff, default_value, modifiable, on_modify,                \
        TRACE_NAME_PREFIX, TRACE_NAME_PREFIX_LEN, TRACE_ENV_NAME_PREFIX,       \
        TRACE_ENV_NAME_PREFIX_LEN, 1)

#ifdef ZTS
#    define DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,   \
        field_name, glob_type, glob_name, name_prefix, name_prefix_len,        \
        env_name, env_name_len, avoid_registering_twice)                       \
        ((dd_ini_setting){(name_suff), (default_value),                        \
            sizeof(name_suff "") - 1, sizeof(default_value "") - 1,            \
            (modifiable), (on_modify), &(glob_name##_id),                      \
            offsetof(glob_type, field_name), name_prefix, name_prefix_len,     \
            env_name, env_name_len, avoid_registering_twice})
#    define DD_APPSEC_INI_ENV_GLOB(name_suff, default_value, modifiable,       \
        on_modify, field_name, glob_type, glob_name)                           \
        DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,       \
            field_name, glob_type, glob_name, APPSEC_NAME_PREFIX,              \
            APPSEC_NAME_PREFIX_LEN, APPSEC_ENV_NAME_PREFIX,                    \
            APPSEC_ENV_NAME_PREFIX_LEN, 0)
#    define DD_TRACE_INI_ENV_GLOB(name_suff, default_value, modifiable,        \
        on_modify, field_name, glob_type, glob_name)                           \
        DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,       \
            field_name, glob_type, glob_name, TRACE_NAME_PREFIX,               \
            TRACE_NAME_PREFIX_LEN, TRACE_ENV_NAME_PREFIX,                      \
            TRACE_ENV_NAME_PREFIX_LEN, 1)
#else
#    define DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,   \
        field_name, glob_type, glob_name, name_prefix, name_prefix_len,        \
        env_name, env_name_len, avoid_registering_twice)                       \
        ((dd_ini_setting){(name_suff), (default_value),                        \
            sizeof(name_suff "") - 1, sizeof(default_value "") - 1,            \
            modifiable, (on_modify), &(glob_name),                             \
            offsetof(glob_type, field_name), name_prefix, name_prefix_len,     \
            env_name, env_name_len, avoid_registering_twice})
#    define DD_APPSEC_INI_ENV_GLOB(name_suff, default_value, modifiable,       \
        on_modify, field_name, glob_type, glob_name)                           \
        DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,       \
            field_name, glob_type, glob_name, APPSEC_NAME_PREFIX,              \
            APPSEC_NAME_PREFIX_LEN, APPSEC_ENV_NAME_PREFIX,                    \
            APPSEC_ENV_NAME_PREFIX_LEN, 0)
#    define DD_TRACE_INI_ENV_GLOB(name_suff, default_value, modifiable,        \
        on_modify, field_name, glob_type, glob_name)                           \
        DD_INI_ENV_GLOB(name_suff, default_value, modifiable, on_modify,       \
            field_name, glob_type, glob_name, TRACE_NAME_PREFIX,               \
            TRACE_NAME_PREFIX_LEN, TRACE_ENV_NAME_PREFIX,                      \
            TRACE_ENV_NAME_PREFIX_LEN, 1)
#endif

void dd_phpobj_startup(int module_number);
dd_result dd_phpobj_reg_funcs(const zend_function_entry *entries);
dd_result dd_phpobj_reg_ini(const zend_ini_entry_def *entries);
void dd_phpobj_reg_ini_env(const dd_ini_setting *sett);
dd_result dd_phpobj_load_env_values(void);
static inline void dd_phpobj_reg_ini_envs(const dd_ini_setting *setts)
{
    for (__auto_type s = setts; s->name_suff; s++) { dd_phpobj_reg_ini_env(s); }
}
void dd_phpobj_reg_long_const(
    const char *name, size_t name_len, zend_long value, int flags);
void dd_phpobj_shutdown(void);
