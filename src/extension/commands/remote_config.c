// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <SAPI.h>
#include <ext/standard/url.h>
#include <php.h>

#include "../commands_helpers.h"
#include "../ddappsec.h"
#include "../ddtrace.h"
#include "../logging.h"
#include "../msgpack_helpers.h"
#include "../php_compat.h"
#include "remote_config.h"
#include <mpack.h>
#include <zend_string.h>

static dd_result _request_pack(
    mpack_writer_t *nonnull w, void *nullable ATTR_UNUSED ctx);
static dd_result _process_response(
    mpack_node_t root, ATTR_UNUSED void *nullable ctx);

static const dd_command_spec _spec = {
    .name = "remote_config",
    .name_len = sizeof("remote_config") - 1,
    .num_args = 1, // a single map
    .outgoing_cb = _request_pack,
    .incoming_cb = _process_response,
};

dd_result dd_remote_config(dd_conn *nonnull conn)
{
    return dd_command_exec(conn, &_spec, NULL);
}

static dd_result _request_pack(
    mpack_writer_t *nonnull w, void *nullable ATTR_UNUSED ctx)
{
    UNUSED(ctx);
    mpack_start_map(w, 0);
    mpack_finish_map(w);

    return dd_success;
}

static dd_result _process_response(
    mpack_node_t root, ATTR_UNUSED void *nullable ctx)
{
    // expected: ['enabled' / 'disabled']
    mpack_node_t verdict = mpack_node_array_at(root, 0);

    bool enabled = dd_mpack_node_lstr_eq(verdict, "enabled");

    return enabled ? dd_enabled : dd_disabled;
}
