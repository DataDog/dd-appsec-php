// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "../../base64.hpp"
#include "parser.hpp"

namespace dds::remote_config {

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
        target_file tf(std::move(path_itr->value.GetString()),
            std::move(raw_itr->value.GetString()));
        output.add_target_file(std::move(tf));
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

        output.add_client_config(std::move(itr->GetString()));
    }

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

    targets_signed ts = output.get_targets().get_target_signed();

    ts.set_version(version_itr->value.GetInt());

    return remote_config_parser_result::success;
}

remote_config_parser_result parse_targets(
    rapidjson::Value::ConstMemberIterator targets_itr,
    get_configs_response &output)
{
    std::string targets_encoded_content = targets_itr->value.GetString();

    if (targets_encoded_content.size() == 0) {
        return remote_config_parser_result::targets_field_empty;
    }

    std::string base64_decoded;
    try {
        base64_decoded =
            dds::remote_config::base64_decode(targets_encoded_content, true);
    } catch (std::runtime_error error) {
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

} // namespace dds::remote_config
