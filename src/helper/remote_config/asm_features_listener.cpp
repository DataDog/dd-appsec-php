// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "asm_features_listener.hpp"
#include "base64.h"
#include "exception.hpp"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

void dds::remote_config::asm_features_listener::on_update(const config &config)
{
    std::string base64_decoded;
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

    auto asm_itr = serialized_doc.FindMember("asm");
    if (asm_itr == serialized_doc.MemberEnd() ||
        asm_itr->value.GetType() != rapidjson::kObjectType) {
        throw error_applying_config("Invalid config json encoded contents: "
                                    "asm key missing or invalid");
    }

    auto enabled_itr = asm_itr->value.FindMember("enabled");
    if (enabled_itr == asm_itr->value.MemberEnd()) {
        throw error_applying_config(
            "Invalid config json encoded contents: enabled key missing");
    }

    if (enabled_itr->value.GetType() == rapidjson::kStringType) {
        if (enabled_itr->value.GetString() == std::string("true")) {
            service_config_->enable_asm();
        } else {
            // This scenario should not happen since RC would remove the file
            // when appsec should not be enabled
            service_config_->disable_asm();
        }
    } else if (enabled_itr->value.GetType() == rapidjson::kTrueType) {
        service_config_->enable_asm();
    } else if (enabled_itr->value.GetType() == rapidjson::kFalseType) {
        // This scenario should not happen since RC would remove the file
        // when appsec should not be enabled
        service_config_->disable_asm();
    } else {
        throw error_applying_config(
            "Invalid config json encoded contents: enabled key invalid");
    }
}
