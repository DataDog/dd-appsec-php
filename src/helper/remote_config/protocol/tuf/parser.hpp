// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "get_configs_response.hpp"

namespace dds::remote_config::protocol {

#define PARSER_RESULTS(X)                                                      \
    X(success)                                                                 \
    X(invalid_json)                                                            \
    X(targets_field_invalid_base64)                                            \
    X(targets_field_invalid_json)                                              \
    X(targets_field_missing)                                                   \
    X(targets_field_invalid_type)                                              \
    X(signed_targets_field_invalid)                                            \
    X(signed_targets_field_missing)                                            \
    X(type_signed_targets_field_invalid)                                       \
    X(type_signed_targets_field_invalid_type)                                  \
    X(type_signed_targets_field_missing)                                       \
    X(version_signed_targets_field_invalid)                                    \
    X(version_signed_targets_field_missing)                                    \
    X(custom_signed_targets_field_invalid)                                     \
    X(custom_signed_targets_field_missing)                                     \
    X(obs_custom_signed_targets_field_invalid)                                 \
    X(obs_custom_signed_targets_field_missing)                                 \
    X(target_files_field_missing)                                              \
    X(target_files_object_invalid)                                             \
    X(target_files_field_invalid_type)                                         \
    X(target_files_path_field_missing)                                         \
    X(target_files_path_field_invalid_type)                                    \
    X(target_files_raw_field_missing)                                          \
    X(target_files_raw_field_invalid_type)                                     \
    X(client_config_field_missing)                                             \
    X(client_config_field_invalid_type)                                        \
    X(client_config_field_invalid_entry)                                       \
    X(targets_signed_targets_field_invalid)                                    \
    X(targets_signed_targets_field_missing)                                    \
    X(custom_path_targets_field_invalid)                                       \
    X(custom_path_targets_field_missing)                                       \
    X(v_path_targets_field_invalid)                                            \
    X(v_path_targets_field_missing)                                            \
    X(hashes_path_targets_field_invalid)                                       \
    X(hashes_path_targets_field_missing)                                       \
    X(hashes_path_targets_field_empty)                                         \
    X(hash_hashes_path_targets_field_invalid)                                  \
    X(length_path_targets_field_invalid)                                       \
    X(length_path_targets_field_missing)

#define RESULT_AS_ENUM_ENTRY(entry) entry,
#define RESULT_AS_CASE(entry)                                                  \
    case remote_config_parser_result::entry:                                   \
        return #entry;

enum class remote_config_parser_result {
    PARSER_RESULTS(RESULT_AS_ENUM_ENTRY) last_one
};

std::string remote_config_parser_result_to_str(
    remote_config_parser_result result);

remote_config_parser_result parse(
    const std::string &body, get_configs_response &output);

} // namespace dds::remote_config::protocol