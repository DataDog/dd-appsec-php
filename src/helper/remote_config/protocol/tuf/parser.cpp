// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "../../../json_helper.hpp"
#include "parser.hpp"

namespace dds::remote_config {

bool validate_field_is_present(const rapidjson::Value &parent_field,
    const char *key, rapidjson::Type type,
    rapidjson::Value::ConstMemberIterator &tmp_itr)
{
    bool result = false;
    tmp_itr = parent_field.FindMember(key);

    bool found = false;
    if (tmp_itr != parent_field.MemberEnd()) {
        found = true;
    }

    if (found && type == tmp_itr->value.GetType()) {
        result = true;
    }

    return result;
}

remote_config_result parser(
    const std::string &body, get_configs_response &output)
{
    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(body).HasParseError()) {
        return remote_config_result::error;
    }

    rapidjson::Value::ConstMemberIterator target_files_itr;
    rapidjson::Value::ConstMemberIterator client_configs_itr;
    rapidjson::Value::ConstMemberIterator targets_itr;

    //Lets validate the data and since we are there we get the iterators
    if (!validate_field_is_present(serialized_doc, "target_files",
            rapidjson::kArrayType, target_files_itr) ||
        !validate_field_is_present(serialized_doc, "client_configs",
            rapidjson::kArrayType, client_configs_itr) ||
        !validate_field_is_present(
            serialized_doc, "targets", rapidjson::kStringType, targets_itr)) {
        return remote_config_result::error;
    }

    return remote_config_result::success;
}

} // namespace dds::remote_config
