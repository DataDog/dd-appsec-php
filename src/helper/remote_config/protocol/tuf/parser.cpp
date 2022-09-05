// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "parser.hpp"
#include <base64.h>

namespace dds::remote_config::protocol {

remote_config_parser_result validate_field_is_present(
    const rapidjson::Value &parent_field, const char *key, rapidjson::Type type,
    rapidjson::Value::ConstMemberIterator &output_itr,
    remote_config_parser_result missing, remote_config_parser_result invalid)
{
    output_itr = parent_field.FindMember(key);

    if (output_itr == parent_field.MemberEnd()) {
        return missing;
    }

    if (type == output_itr->value.GetType()) {
        return remote_config_parser_result::success;
    }

    return invalid;
}

remote_config_parser_result validate_field_is_present(
    rapidjson::Value::ConstMemberIterator &parent_field, const char *key,
    rapidjson::Type type, rapidjson::Value::ConstMemberIterator &output_itr,
    remote_config_parser_result missing, remote_config_parser_result invalid)
{
    output_itr = parent_field->value.FindMember(key);

    if (output_itr == parent_field->value.MemberEnd()) {
        return missing;
    }

    if (type == output_itr->value.GetType()) {
        return remote_config_parser_result::success;
    }

    return invalid;
}

std::pair<remote_config_parser_result, std::vector<target_file>>
parse_target_files(rapidjson::Value::ConstMemberIterator target_files_itr)
{
    std::pair<remote_config_parser_result, std::vector<target_file>> result;
    for (rapidjson::Value::ConstValueIterator itr =
             target_files_itr->value.Begin();
         itr != target_files_itr->value.End(); ++itr) {
        if (!itr->IsObject()) {
            result.first =
                remote_config_parser_result::target_files_object_invalid;
            return result;
        }

        // Path checks
        rapidjson::Value::ConstMemberIterator path_itr =
            itr->GetObject().FindMember("path");
        if (path_itr == itr->GetObject().MemberEnd()) {
            result.first =
                remote_config_parser_result::target_files_path_field_missing;
            return result;
        }
        if (!path_itr->value.IsString()) {
            result.first = remote_config_parser_result::
                target_files_path_field_invalid_type;
            return result;
        }

        // Raw checks
        rapidjson::Value::ConstMemberIterator raw_itr =
            itr->GetObject().FindMember("raw");
        if (raw_itr == itr->GetObject().MemberEnd()) {
            result.first =
                remote_config_parser_result::target_files_raw_field_missing;
            return result;
        }
        if (!raw_itr->value.IsString()) {
            result.first = remote_config_parser_result::
                target_files_raw_field_invalid_type;
            return result;
        }
        std::string path(path_itr->value.GetString());
        std::string raw(raw_itr->value.GetString());
        result.second.emplace_back(path, raw);
    }

    result.first = remote_config_parser_result::success;
    return result;
}

std::pair<remote_config_parser_result, std::vector<std::string>>
parse_client_configs(rapidjson::Value::ConstMemberIterator client_configs_itr)
{
    std::pair<remote_config_parser_result, std::vector<std::string>> result;
    for (rapidjson::Value::ConstValueIterator itr =
             client_configs_itr->value.Begin();
         itr != client_configs_itr->value.End(); ++itr) {
        if (!itr->IsString()) {
            result.first =
                remote_config_parser_result::client_config_field_invalid_entry;
            return result;
        }

        result.second.emplace_back(itr->GetString());
    }

    result.first = remote_config_parser_result::success;
    return result;
}

std::pair<remote_config_parser_result,
    std::pair<std::string, std::optional<path>>>
parse_target(rapidjson::Value::ConstMemberIterator target_itr)
{
    std::pair<remote_config_parser_result,
        std::pair<std::string, std::optional<path>>>
        result;
    rapidjson::Value::ConstMemberIterator custom_itr;
    result.first = validate_field_is_present(target_itr, "custom",
        rapidjson::kObjectType, custom_itr,
        remote_config_parser_result::custom_path_targets_field_missing,
        remote_config_parser_result::custom_path_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }
    rapidjson::Value::ConstMemberIterator v_itr;
    result.first =
        validate_field_is_present(custom_itr, "v", rapidjson::kNumberType,
            v_itr, remote_config_parser_result::v_path_targets_field_missing,
            remote_config_parser_result::v_path_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    rapidjson::Value::ConstMemberIterator hashes_itr;
    result.first = validate_field_is_present(target_itr, "hashes",
        rapidjson::kObjectType, hashes_itr,
        remote_config_parser_result::hashes_path_targets_field_missing,
        remote_config_parser_result::hashes_path_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    std::map<std::string, std::string> hashes_mapped;
    auto hashes_object = hashes_itr->value.GetObject();
    for (rapidjson::Value::ConstMemberIterator itr =
             hashes_object.MemberBegin();
         itr != hashes_object.MemberEnd(); ++itr) {
        if (itr->value.GetType() != rapidjson::kStringType) {
            result.first = remote_config_parser_result::
                hash_hashes_path_targets_field_invalid;
            return result;
        }

        std::pair<std::string, std::string> hash_pair(
            itr->name.GetString(), itr->value.GetString());
        hashes_mapped.insert(hash_pair);
    }

    if (hashes_mapped.empty()) {
        result.first =
            remote_config_parser_result::hashes_path_targets_field_empty;
        return result;
    }

    rapidjson::Value::ConstMemberIterator length_itr;
    result.first = validate_field_is_present(target_itr, "length",
        rapidjson::kNumberType, length_itr,
        remote_config_parser_result::length_path_targets_field_missing,
        remote_config_parser_result::length_path_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    std::string target_name(target_itr->name.GetString());
    path path_object(
        v_itr->value.GetInt(), hashes_mapped, length_itr->value.GetInt());
    result.second.first = target_name;
    result.second.second = path_object;
    result.first = remote_config_parser_result::success;

    return result;
}

std::pair<remote_config_parser_result, std::optional<targets>>
parse_targets_signed(rapidjson::Value::ConstMemberIterator targets_signed_itr)
{
    std::pair<remote_config_parser_result, std::optional<targets>> result;
    rapidjson::Value::ConstMemberIterator version_itr;
    result.first = validate_field_is_present(targets_signed_itr, "version",
        rapidjson::kNumberType, version_itr,
        remote_config_parser_result::version_signed_targets_field_missing,
        remote_config_parser_result::version_signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    rapidjson::Value::ConstMemberIterator targets_itr;
    result.first = validate_field_is_present(targets_signed_itr, "targets",
        rapidjson::kObjectType, targets_itr,
        remote_config_parser_result::targets_signed_targets_field_missing,
        remote_config_parser_result::targets_signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    std::vector<std::pair<std::string, std::optional<path>>> paths;
    for (rapidjson::Value::ConstMemberIterator current_target =
             targets_itr->value.MemberBegin();
         current_target != targets_itr->value.MemberEnd(); ++current_target) {
        auto [result_parse_target, p] = parse_target(current_target);
        paths.push_back(p);
        result.first = result_parse_target;
        if (result.first != remote_config_parser_result::success) {
            return result;
        }
    }

    rapidjson::Value::ConstMemberIterator type_itr;
    result.first = validate_field_is_present(targets_signed_itr, "_type",
        rapidjson::kStringType, type_itr,
        remote_config_parser_result::type_signed_targets_field_missing,
        remote_config_parser_result::type_signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }
    if (strcmp(type_itr->value.GetString(), "targets") != 0) {
        result.first =
            remote_config_parser_result::type_signed_targets_field_invalid_type;
        return result;
    }

    rapidjson::Value::ConstMemberIterator custom_itr;
    result.first = validate_field_is_present(targets_signed_itr, "custom",
        rapidjson::kObjectType, custom_itr,
        remote_config_parser_result::custom_signed_targets_field_missing,
        remote_config_parser_result::custom_signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    rapidjson::Value::ConstMemberIterator opaque_backend_state_itr;
    result.first = validate_field_is_present(custom_itr, "opaque_backend_state",
        rapidjson::kStringType, opaque_backend_state_itr,
        remote_config_parser_result::obs_custom_signed_targets_field_missing,
        remote_config_parser_result::obs_custom_signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }
    std::string obs = opaque_backend_state_itr->value.GetString();
    std::vector<std::pair<std::string, path>> final_paths;
    final_paths.reserve(paths.size());
    for (auto &[product_str, path] : paths) {
        final_paths.emplace_back(product_str, path.value());
    }
    result.second = targets(version_itr->value.GetInt(), obs, final_paths);
    result.first = remote_config_parser_result::success;

    return result;
}

std::pair<remote_config_parser_result, std::optional<targets>> parse_targets(
    rapidjson::Value::ConstMemberIterator targets_itr)
{
    std::string targets_encoded_content = targets_itr->value.GetString();
    std::pair<remote_config_parser_result, std::optional<targets>> result;

    if (targets_encoded_content.empty()) {
        result.first = remote_config_parser_result::success;
        return result;
    }

    std::string base64_decoded;
    try {
        base64_decoded = base64_decode(targets_encoded_content, true);
    } catch (std::runtime_error &error) {
        result.first =
            remote_config_parser_result::targets_field_invalid_base64;
        return result;
    }

    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(base64_decoded).HasParseError()) {
        result.first = remote_config_parser_result::targets_field_invalid_json;
        return result;
    }

    rapidjson::Value::ConstMemberIterator signed_itr;

    // Lets validate the data and since we are there we get the iterators
    result.first = validate_field_is_present(serialized_doc, "signed",
        rapidjson::kObjectType, signed_itr,
        remote_config_parser_result::signed_targets_field_missing,
        remote_config_parser_result::signed_targets_field_invalid);
    if (result.first != remote_config_parser_result::success) {
        return result;
    }

    result = parse_targets_signed(signed_itr);

    return result;
}

std::pair<remote_config_parser_result, std::optional<get_configs_response>>
parse(const std::string &body)
{
    rapidjson::Document serialized_doc;
    std::pair<remote_config_parser_result, std::optional<get_configs_response>>
        parser_result;
    if (serialized_doc.Parse(body).HasParseError()) {
        parser_result.first = remote_config_parser_result::invalid_json;
        return parser_result;
    }

    rapidjson::Value::ConstMemberIterator target_files_itr;
    rapidjson::Value::ConstMemberIterator client_configs_itr;
    rapidjson::Value::ConstMemberIterator targets_itr;

    // Lets validate the data and since we are there we get the iterators
    parser_result.first = validate_field_is_present(serialized_doc,
        "target_files", rapidjson::kArrayType, target_files_itr,
        remote_config_parser_result::target_files_field_missing,
        remote_config_parser_result::target_files_field_invalid_type);
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }
    parser_result.first = validate_field_is_present(serialized_doc,
        "client_configs", rapidjson::kArrayType, client_configs_itr,
        remote_config_parser_result::client_config_field_missing,
        remote_config_parser_result::client_config_field_invalid_type);
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }
    parser_result.first = validate_field_is_present(serialized_doc, "targets",
        rapidjson::kStringType, targets_itr,
        remote_config_parser_result::targets_field_missing,
        remote_config_parser_result::targets_field_invalid_type);
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }

    auto [result_parse_tf, target_files] = parse_target_files(target_files_itr);
    parser_result.first = result_parse_tf;
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }

    auto [result_parse_cc, client_configs] =
        parse_client_configs(client_configs_itr);
    parser_result.first = result_parse_cc;
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }

    auto [result_parse_targets, targets] = parse_targets(targets_itr);
    parser_result.first = result_parse_targets;
    if (parser_result.first != remote_config_parser_result::success) {
        return parser_result;
    }

    parser_result.second =
        get_configs_response(client_configs, target_files, targets.value());

    return parser_result;
}
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RESULT_AS_STR(entry) #entry,
constexpr static std::array<std::string_view,
    (size_t)remote_config_parser_result::num_of_values>
    results_as_str = {PARSER_RESULTS(RESULT_AS_STR)};
std::string_view remote_config_parser_result_to_str(
    remote_config_parser_result result)
{
    if (result == remote_config_parser_result::num_of_values) {
        return "";
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return results_as_str[(size_t)result];
}

} // namespace dds::remote_config::protocol
