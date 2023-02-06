// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "asm_data_listener.hpp"
#include "../json_helper.hpp"
#include "exception.hpp"
#include <optional>
#include <rapidjson/document.h>

struct rule_data {
    struct data_with_expiration {
        std::string_view value;
        int expiration;
    };

    std::string_view id;
    std::string_view type;
    std::map<std::string_view, data_with_expiration> data;
};

void extract_data(
    rapidjson::Value::ConstMemberIterator itr, rule_data &rule_data)
{
    for (const auto *data_entry_itr = itr->value.Begin();
         data_entry_itr != itr->value.End(); ++data_entry_itr) {
        if (!data_entry_itr->IsObject()) {
            throw dds::remote_config::error_applying_config(
                "Invalid config json contents: "
                "Entry on data not a valid object");
        }

        auto expiration = dds::json_helper::get_field_of_type(
            data_entry_itr, "expiration", rapidjson::kNumberType);
        auto value = dds::json_helper::get_field_of_type(
            data_entry_itr, "value", rapidjson::kStringType);
        if (!expiration || !value) {
            throw dds::remote_config::error_applying_config(
                "Invalid content of data entry");
        }

        auto previous = rule_data.data.find(value.value()->value.GetString());
        if (previous != rule_data.data.end()) {
            if (previous->second.expiration <
                expiration.value()->value.GetInt()) {
                previous->second.expiration =
                    expiration.value()->value.GetInt();
            }
        } else {
            rule_data.data.insert({value.value()->value.GetString(),
                {value.value()->value.GetString(),
                    expiration.value()->value.GetInt()}});
        }
    }
}

dds::parameter rules_to_parameter(
    const std::unordered_map<std::string, rule_data> &rules)
{
    dds::parameter parameter = dds::parameter::map();
    dds::parameter rules_data = dds::parameter::array();
    for (const auto &[key, rule] : rules) {
        dds::parameter parameter_rule = dds::parameter::map();

        parameter_rule.add("id", dds::parameter::string(rule.id));
        parameter_rule.add("type", dds::parameter::string(rule.type));

        // Data
        dds::parameter data = dds::parameter::array();
        for (const auto &[value, data_entry] : rule.data) {
            dds::parameter data_parameter = dds::parameter::map();
            data_parameter.add("expiration",
                dds::parameter::uint64(data_entry.expiration, false));
            data_parameter.add("value", dds::parameter::string(value));
            data.add(std::move(data_parameter));
        }
        parameter_rule.add("data", std::move(data));

        rules_data.add(std::move(parameter_rule));
    }

    parameter.add("rules_data", std::move(rules_data));

    return parameter;
}

void dds::remote_config::asm_data_listener::on_update(const config &config)
{
    std::unordered_map<std::string, rule_data> rules_data = {};
    rapidjson::Document serialized_doc;
    if (!json_helper::get_json_base64_encoded_content(
            config.contents, serialized_doc)) {
        throw error_applying_config("Invalid config contents");
    }

    auto rules_data_itr = json_helper::get_field_of_type(
        serialized_doc, "rules_data", rapidjson::kArrayType);
    if (!rules_data_itr) {
        throw error_applying_config("Invalid config json contents: "
                                    "rules_data key missing or invalid");
    }

    for (rapidjson::Value::ConstValueIterator itr =
             rules_data_itr.value()->value.Begin();
         itr != rules_data_itr.value()->value.End(); ++itr) {
        if (!itr->IsObject()) {
            throw error_applying_config("Invalid config json contents: "
                                        "rules_data entry invalid");
        }

        auto id_itr =
            json_helper::get_field_of_type(itr, "id", rapidjson::kStringType);
        auto type_itr =
            json_helper::get_field_of_type(itr, "type", rapidjson::kStringType);
        auto data_itr =
            json_helper::get_field_of_type(itr, "data", rapidjson::kArrayType);
        if (!id_itr || !type_itr || !data_itr) {
            throw error_applying_config(
                "Invalid config json contents: "
                "rules_data missing a field or field is invalid");
        }

        auto rule = rules_data.find(id_itr.value()->value.GetString());
        if (rule == rules_data.end()) { // New rule
            rule_data new_rule_data = {id_itr.value()->value.GetString(),
                type_itr.value()->value.GetString()};
            extract_data(data_itr.value(), new_rule_data);
            if (!new_rule_data.data.empty()) {
                rules_data.insert(
                    {id_itr.value()->value.GetString(), new_rule_data});
            }
        } else {
            extract_data(data_itr.value(), rule->second);
        }
    }

    auto parameter = rules_to_parameter(rules_data);
    auto parameter_view = dds::parameter_view(parameter);

    engine_->update_rule_data(parameter_view);
}
