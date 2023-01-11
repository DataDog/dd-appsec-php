// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.
#pragma once
#include "attributes.h"
#include <php.h>
#include <zend.h>
#include <stdbool.h>

#define DD_TAG_DATA_MAX_LEN (1024UL * 1024UL)

void dd_tags_startup(void);
void dd_tags_shutdown(void);
void dd_tags_rinit(void);
void dd_tags_rshutdown(void);
void dd_tags_rshutdown_testing(void);

void dd_tags_set_sampling_priority(void);

// does not increase the refcount on zstr
void dd_tags_add_appsec_json_frag(zend_string *nonnull zstr);

// Event tracking functions
PHP_FUNCTION(datadog_appsec_track_user_login_event);
PHP_FUNCTION(datadog_appsec_track_custom_event);
