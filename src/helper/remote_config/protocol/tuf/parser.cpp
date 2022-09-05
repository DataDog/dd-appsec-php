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

remote_config_parser_result parse_target_files(
    rapidjson::Value::ConstMemberIterator target_files_itr,
    get_configs_response &output)
{
    for (rapidjson::Value::ConstValueIterator itr =
             target_files_itr->value.Begin();
         itr != target_files_itr->value.End(); ++itr) {
        if (!itr->IsObject()) {
            return remote_config_parser_result::target_files_object_invalid;
        }

        // Path checks
        rapidjson::Value::ConstMemberIterator path_itr =
            itr->GetObject().FindMember("path");
        if (path_itr == itr->GetObject().MemberEnd()) {
            return remote_config_parser_result::target_files_path_field_missing;
        }
        if (!path_itr->value.IsString()) {
            return remote_config_parser_result::
                target_files_path_field_invalid_type;
        }

        // Raw checks
        rapidjson::Value::ConstMemberIterator raw_itr =
            itr->GetObject().FindMember("raw");
        if (raw_itr == itr->GetObject().MemberEnd()) {
            return remote_config_parser_result::target_files_raw_field_missing;
        }
        if (!raw_itr->value.IsString()) {
            return remote_config_parser_result::
                target_files_raw_field_invalid_type;
        }
        std::string path(path_itr->value.GetString());
        std::string raw(raw_itr->value.GetString());
        target_file tf(path, raw);
        output.add_target_file(tf);
    }

    return remote_config_parser_result::success;
}

remote_config_parser_result parse_client_configs(
    rapidjson::Value::ConstMemberIterator client_configs_itr,
    get_configs_response &output)
{
    for (rapidjson::Value::ConstValueIterator itr =
             client_configs_itr->value.Begin();
         itr != client_configs_itr->value.End(); ++itr) {
        if (!itr->IsString()) {
            return remote_config_parser_result::
                client_config_field_invalid_entry;
        }

        std::string client_c(itr->GetString());
        output.add_client_config(client_c);
    }

    return remote_config_parser_result::success;
}

remote_config_parser_result parse_target(
    rapidjson::Value::ConstMemberIterator target_itr, targets *output)
{
    rapidjson::Value::ConstMemberIterator custom_itr;
    remote_config_parser_result result = validate_field_is_present(target_itr,
        "custom", rapidjson::kObjectType, custom_itr,
        remote_config_parser_result::custom_path_targets_field_missing,
        remote_config_parser_result::custom_path_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }
    rapidjson::Value::ConstMemberIterator v_itr;
    result = validate_field_is_present(custom_itr, "v", rapidjson::kNumberType,
        v_itr, remote_config_parser_result::v_path_targets_field_missing,
        remote_config_parser_result::v_path_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    rapidjson::Value::ConstMemberIterator hashes_itr;
    result = validate_field_is_present(target_itr, "hashes",
        rapidjson::kObjectType, hashes_itr,
        remote_config_parser_result::hashes_path_targets_field_missing,
        remote_config_parser_result::hashes_path_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    std::map<std::string, std::string> hashes_mapped;
    auto hashes_object = hashes_itr->value.GetObject();
    for (rapidjson::Value::ConstMemberIterator itr =
             hashes_object.MemberBegin();
         itr != hashes_object.MemberEnd(); ++itr) {
        if (itr->value.GetType() != rapidjson::kStringType) {
            return remote_config_parser_result::
                hash_hashes_path_targets_field_invalid;
        }

        std::pair<std::string, std::string> hash_pair(
            itr->name.GetString(), itr->value.GetString());
        hashes_mapped.insert(hash_pair);
    }

    if (hashes_mapped.empty()) {
        return remote_config_parser_result::hashes_path_targets_field_empty;
    }

    rapidjson::Value::ConstMemberIterator length_itr;
    result = validate_field_is_present(target_itr, "length",
        rapidjson::kNumberType, length_itr,
        remote_config_parser_result::length_path_targets_field_missing,
        remote_config_parser_result::length_path_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    std::string target_name(target_itr->name.GetString());
    path path_object(
        v_itr->value.GetInt(), hashes_mapped, length_itr->value.GetInt());
    output->add_path(target_name, path_object);

    return remote_config_parser_result::success;
}

remote_config_parser_result parse_targets_signed(
    rapidjson::Value::ConstMemberIterator targets_signed_itr,
    get_configs_response &output)
{

    rapidjson::Value::ConstMemberIterator version_itr;
    remote_config_parser_result result = validate_field_is_present(
        targets_signed_itr, "version", rapidjson::kNumberType, version_itr,
        remote_config_parser_result::version_signed_targets_field_missing,
        remote_config_parser_result::version_signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    targets *_targets = output.get_targets();
    _targets->set_version(version_itr->value.GetInt());

    rapidjson::Value::ConstMemberIterator targets_itr;
    result = validate_field_is_present(targets_signed_itr, "targets",
        rapidjson::kObjectType, targets_itr,
        remote_config_parser_result::targets_signed_targets_field_missing,
        remote_config_parser_result::targets_signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    for (rapidjson::Value::ConstMemberIterator current_target =
             targets_itr->value.MemberBegin();
         current_target != targets_itr->value.MemberEnd(); ++current_target) {
        result = parse_target(current_target, output.get_targets());
        if (result != remote_config_parser_result::success) {
            return result;
        }
    }

    rapidjson::Value::ConstMemberIterator type_itr;
    result = validate_field_is_present(targets_signed_itr, "_type",
        rapidjson::kStringType, type_itr,
        remote_config_parser_result::type_signed_targets_field_missing,
        remote_config_parser_result::type_signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }
    if (strcmp(type_itr->value.GetString(), "targets") != 0) {
        return remote_config_parser_result::
            type_signed_targets_field_invalid_type;
    }

    rapidjson::Value::ConstMemberIterator custom_itr;
    result = validate_field_is_present(targets_signed_itr, "custom",
        rapidjson::kObjectType, custom_itr,
        remote_config_parser_result::custom_signed_targets_field_missing,
        remote_config_parser_result::custom_signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    rapidjson::Value::ConstMemberIterator opaque_backend_state_itr;
    result = validate_field_is_present(custom_itr, "opaque_backend_state",
        rapidjson::kStringType, opaque_backend_state_itr,
        remote_config_parser_result::obs_custom_signed_targets_field_missing,
        remote_config_parser_result::obs_custom_signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }
    std::string obs = opaque_backend_state_itr->value.GetString();
    _targets->set_opaque_backend_state(obs);

    return remote_config_parser_result::success;
}

remote_config_parser_result parse_targets(
    rapidjson::Value::ConstMemberIterator targets_itr,
    get_configs_response &output)
{
    std::string targets_encoded_content = targets_itr->value.GetString();

    if (targets_encoded_content.empty()) {
        return remote_config_parser_result::success;
    }

    std::string base64_decoded;
    try {
        base64_decoded = base64_decode(targets_encoded_content, true);
    } catch (std::runtime_error &error) {
        return remote_config_parser_result::targets_field_invalid_base64;
    }

    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(base64_decoded).HasParseError()) {
        return remote_config_parser_result::targets_field_invalid_json;
    }

    rapidjson::Value::ConstMemberIterator signed_itr;

    // Lets validate the data and since we are there we get the iterators
    remote_config_parser_result result = validate_field_is_present(
        serialized_doc, "signed", rapidjson::kObjectType, signed_itr,
        remote_config_parser_result::signed_targets_field_missing,
        remote_config_parser_result::signed_targets_field_invalid);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    result = parse_targets_signed(signed_itr, output);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    return remote_config_parser_result::success;
}

remote_config_parser_result parse(
    const std::string &body, get_configs_response &output)
{
    remote_config_parser_result result;
    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(body).HasParseError()) {
        return remote_config_parser_result::invalid_json;
    }

    rapidjson::Value::ConstMemberIterator target_files_itr;
    rapidjson::Value::ConstMemberIterator client_configs_itr;
    rapidjson::Value::ConstMemberIterator targets_itr;

    // Lets validate the data and since we are there we get the iterators
    result = validate_field_is_present(serialized_doc, "target_files",
        rapidjson::kArrayType, target_files_itr,
        remote_config_parser_result::target_files_field_missing,
        remote_config_parser_result::target_files_field_invalid_type);
    if (result != remote_config_parser_result::success) {
        return result;
    }
    result = validate_field_is_present(serialized_doc, "client_configs",
        rapidjson::kArrayType, client_configs_itr,
        remote_config_parser_result::client_config_field_missing,
        remote_config_parser_result::client_config_field_invalid_type);
    if (result != remote_config_parser_result::success) {
        return result;
    }
    result = validate_field_is_present(serialized_doc, "targets",
        rapidjson::kStringType, targets_itr,
        remote_config_parser_result::targets_field_missing,
        remote_config_parser_result::targets_field_invalid_type);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    result = parse_target_files(target_files_itr, output);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    result = parse_client_configs(client_configs_itr, output);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    result = parse_targets(targets_itr, output);
    if (result != remote_config_parser_result::success) {
        return result;
    }

    return remote_config_parser_result::success;
}

std::string remote_config_parser_result_to_str(
    remote_config_parser_result result)
{
    switch (result) {
        PARSER_RESULTS(RESULT_AS_CASE)
    default:
        return "";
    }
};

} // namespace dds::remote_config::protocol