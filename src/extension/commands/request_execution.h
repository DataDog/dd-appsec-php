// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "../network.h"

dd_result dd_request_execution(dd_conn *nonnull conn);
void dd_request_execution_add_data(zend_string *key, zval *data);
void dd_request_execution_rinit(void);
void dd_request_execution_rshutdown(void);
