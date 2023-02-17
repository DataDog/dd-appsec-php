// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "request_execution.h"
#include "../commands_helpers.h"
#include "../msgpack_helpers.h"
#include "mpack-common.h"
#include "mpack-node.h"
#include "mpack-writer.h"
#include <php.h>
#include <zend_hash.h>
#include <zend_types.h>

static THREAD_LOCAL_ON_ZTS zval data;

static dd_result _pack_command(
    mpack_writer_t *nonnull w, ATTR_UNUSED void *nullable ctx);

static const dd_command_spec _spec = {
    .name = "request_execution",
    .name_len = sizeof("request_execution") - 1,
    .num_args = 1, // a single map
    .outgoing_cb = _pack_command,
    .incoming_cb = dd_command_proc_resp_verd_span_data,
    .config_features_cb = dd_command_process_config_features_unexpected,
};

dd_result dd_request_execution(dd_conn *nonnull conn)
{
    return dd_command_exec(conn, &_spec, NULL);
}

static bool _prepare_data(void)
{
    if (Z_TYPE(data) == IS_UNDEF) {
        array_init(&data);
    }

    return Z_TYPE(data) == IS_ARRAY;
}

static dd_result _pack_command(
    mpack_writer_t *nonnull w, ATTR_UNUSED void *nullable ctx)
{
    UNUSED(ctx);
    _prepare_data();
    dd_mpack_write_zval(w, &data);

    return dd_success;
}

void dd_request_execution_rshutdown(void)
{
    if (Z_TYPE(data) == IS_ARRAY) {
        zend_array_destroy(Z_ARR(data));
    }
}

void dd_request_execution_add_data(char *key, zval *value)
{
    if (!_prepare_data()) {
        return;
    }
    add_assoc_zval(&data, key, value);
}