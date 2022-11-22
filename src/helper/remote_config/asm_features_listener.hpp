// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "base64.h"
#include "config.hpp"
#include "exception.hpp"
#include "listener.hpp"
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace dds::remote_config {

class asm_features_listener : public product_listener_base {
public:
    asm_features_listener(std::shared_ptr<remote_config::remote_config_service>
            remote_config_service)
        : product_listener_base(remote_config_service){};
    void on_update(const config &config) override
    {
        std::string base64_decoded;
        try {
            base64_decoded = base64_decode(config.contents, true);
        } catch (std::runtime_error &error) {
            throw error_applying_config(
                "Invalid config base64 encoded contents: " +
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
        if (enabled_itr == asm_itr->value.MemberEnd() ||
            enabled_itr->value.GetType() != rapidjson::kStringType) {
            throw error_applying_config(
                "Invalid config json encoded contents: enabled key missing");
        }

        if (strcmp("true", enabled_itr->value.GetString()) == 0) {
            _remote_config_service->enable_asm();
        } else {
            _remote_config_service->disable_asm();
        }
    }

    void on_unapply(const config & /*config*/) override
    {
        _remote_config_service->disable_asm();
    }
};

} // namespace dds::remote_config
