// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "base64.h"
#include "config.hpp"
#include <optional>
#include <rapidjson/document.h>

namespace dds::remote_config {

class error_applying_config : public std::exception {
private:
    std::string message;

public:
    explicit error_applying_config(std::string &&msg) : message(std::move(msg))
    {}
    std::string what() { return message; }
};

class product_listener_base {
public:
    product_listener_base() = default;
    product_listener_base(const product_listener_base &) = default;
    product_listener_base(product_listener_base &&) = default;
    product_listener_base &operator=(const product_listener_base &) = default;
    product_listener_base &operator=(product_listener_base &&) = default;
    virtual ~product_listener_base() = default;

    virtual void on_update(const config &config) = 0;
    virtual void on_unapply(const config &config) = 0;
};

class asm_features_listener : public product_listener_base {
public:
    asm_features_listener() : active(false) {}
    void on_update(const config &config)
    {
        std::string base64_decoded;
        try {
            base64_decoded = base64_decode(config.contents, true);
        } catch (std::runtime_error &error) {
            throw error_applying_config(
                "Invalid config base64 encoded contents");
        }

        rapidjson::Document serialized_doc;
        if (serialized_doc.Parse(base64_decoded).HasParseError()) {
            throw error_applying_config("Invalid config json contents");
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
            throw error_applying_config("Invalid config json encoded contents: "
                                        "enabled key missing");
        }

        active = strcmp("true", enabled_itr->value.GetString()) == 0;
    }

    void on_unapply(const config &config) { active = false; }

    bool is_active() { return active; }

private:
    bool active;
};

} // namespace dds::remote_config
