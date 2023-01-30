// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "asm_data_listener.hpp"
#include "base64.h"
#include "exception.hpp"
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

void dds::remote_config::asm_data_listener::on_update(const config &config)
{
    std::string base64_decoded;
    std::unordered_map<std::string, dds::service_config::rule_data> rules_data =
        {};
    try {
        base64_decoded = base64_decode(config.contents, true);
    } catch (std::runtime_error &error) {
        throw error_applying_config("Invalid config base64 encoded contents: " +
                                    std::string(error.what()));
    }

    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(base64_decoded).HasParseError()) {
        throw error_applying_config("Invalid config json contents: " +
                                    std::string(rapidjson::GetParseError_En(
                                        serialized_doc.GetParseError())));
    }

    auto rules_data_itr = serialized_doc.FindMember("rules_data");
    if (rules_data_itr == serialized_doc.MemberEnd() ||
        !rules_data_itr->value.IsArray()) {
        throw error_applying_config("Invalid config json contents: "
                                    "rules_data key missing or invalid");
    }

    for (auto *itr = rules_data_itr->value.Begin();
         itr != rules_data_itr->value.End(); ++itr) {
        if (!itr->IsObject()) {
            throw error_applying_config("Invalid config json contents: "
                                        "rules_data entry invalid");
        }
        auto id_itr = itr->FindMember("id");
        auto type_itr = itr->FindMember("type");
        auto data_itr = itr->FindMember("data");

        if (id_itr == itr->MemberEnd() || type_itr == itr->MemberEnd() ||
            data_itr == itr->MemberEnd() || !id_itr->value.IsString() ||
            !type_itr->value.IsString() || !data_itr->value.IsArray()) {
            throw error_applying_config(
                "Invalid config json contents: "
                "rules_data missing a field or field is invalid");
        }

        auto rule = rules_data.find(id_itr->value.GetString());
        // Data parsing
        std::map<std::string,
            dds::service_config::rule_data::data_with_expiration>
            new_set;
        std::map<std::string,
            dds::service_config::rule_data::data_with_expiration> &data =
            rule != rules_data.end() ? rule->second.data : new_set;

        for (auto *data_entry_itr = data_itr->value.Begin();
             data_entry_itr != data_itr->value.End(); ++data_entry_itr) {
            if (!data_entry_itr->IsObject()) {
                throw error_applying_config("Invalid config json contents: "
                                            "Entry on data not a valid object");
            }

            auto expiration = data_entry_itr->FindMember("expiration");
            auto value = data_entry_itr->FindMember("value");
            if (!expiration->value.IsNumber() || !value->value.IsString()) {
                continue;
            }

            auto previous = data.find(value->value.GetString());
            if (previous != data.end()) {
                if (previous->second.expiration < expiration->value.GetInt()) {
                    previous->second.expiration = expiration->value.GetInt();
                }
            } else {
                data.insert({value->value.GetString(),
                    {value->value.GetString(), expiration->value.GetInt()}});
            }
        }

        if (data.empty()) {
            continue;
        }

        // New set
        if (rule == rules_data.end()) {
            rules_data.insert({id_itr->value.GetString(),
                {id_itr->value.GetString(), type_itr->value.GetString(),
                    data}});
        }
    }

    service_config_->set_rules_data(std::move(rules_data));
}
