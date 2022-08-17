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

enum class remote_config_parser_result {
    success,
    invalid_json,
    targets_field_empty,
    targets_field_invalid_base64,
    targets_field_invalid_json,
    targets_field_missing,
    targets_field_invalid_type,
    signed_targets_field_invalid,
    signed_targets_field_missing,
    version_signed_targets_field_invalid,
    version_signed_targets_field_missing,
    custom_signed_targets_field_invalid,
    custom_signed_targets_field_missing,
    obs_custom_signed_targets_field_invalid,
    obs_custom_signed_targets_field_missing,
    target_files_field_missing,
    target_files_object_invalid,
    target_files_field_invalid_type,
    target_files_path_field_missing,
    target_files_path_field_invalid_type,
    target_files_raw_field_missing,
    target_files_raw_field_invalid_type,
    client_config_field_missing,
    client_config_field_invalid_type,
    client_config_field_invalid_entry,
    targets_signed_targets_field_invalid,
    targets_signed_targets_field_missing,
    custom_path_targets_field_invalid,
    custom_path_targets_field_missing,
    v_path_targets_field_invalid,
    v_path_targets_field_missing,
    hashes_path_targets_field_invalid,
    hashes_path_targets_field_missing,
    sha256_path_targets_field_invalid,
    sha256_path_targets_field_missing,
    length_path_targets_field_invalid,
    length_path_targets_field_missing
};

std::string remote_config_parser_result_to_str(
    remote_config_parser_result result);

remote_config_parser_result parse(
    const std::string &body, get_configs_response &output);

} // namespace dds::remote_config::protocol