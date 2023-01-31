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

void dds::remote_config::asm_data_listener::on_update(const config &config)
{
    std::unordered_map<std::string, dds::service_config::rule_data> rules_data =
        {};
    std::optional<rapidjson::Document> serialized_doc =
        json_helper::get_json_base64_encoded_content(config.contents);

    if (!serialized_doc) {
        throw error_applying_config("Invalid config contents");
    }

    auto rules_data_itr = json_helper::get_field_of_type(
        serialized_doc.value(), "rules_data", rapidjson::kArrayType);
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
        // Data parsing
        std::map<std::string,
            dds::service_config::rule_data::data_with_expiration>
            new_set;
        std::map<std::string,
            dds::service_config::rule_data::data_with_expiration> &data =
            rule != rules_data.end() ? rule->second.data : new_set;

        for (const auto *data_entry_itr = data_itr.value()->value.Begin();
             data_entry_itr != data_itr.value()->value.End();
             ++data_entry_itr) {
            if (!data_entry_itr->IsObject()) {
                throw error_applying_config("Invalid config json contents: "
                                            "Entry on data not a valid object");
            }

            auto expiration = json_helper::get_field_of_type(
                data_entry_itr, "expiration", rapidjson::kNumberType);
            auto value = json_helper::get_field_of_type(
                data_entry_itr, "value", rapidjson::kStringType);
            if (!expiration || !value) {
                throw error_applying_config("Invalid content of data entry");
            }

            auto previous = data.find(value.value()->value.GetString());
            if (previous != data.end()) {
                if (previous->second.expiration <
                    expiration.value()->value.GetInt()) {
                    previous->second.expiration =
                        expiration.value()->value.GetInt();
                }
            } else {
                data.insert({value.value()->value.GetString(),
                    {value.value()->value.GetString(),
                        expiration.value()->value.GetInt()}});
            }
        }

        if (data.empty()) {
            continue;
        }

        // New set
        if (rule == rules_data.end()) {
            rules_data.insert({id_itr.value()->value.GetString(),
                {id_itr.value()->value.GetString(),
                    type_itr.value()->value.GetString(), data}});
        }
    }

    service_config_->set_rules_data(std::move(rules_data));
}
