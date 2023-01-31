// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include "listener.hpp"
#include <optional>
#include <rapidjson/document.h>

namespace dds::remote_config {

class asm_data_listener : public product_listener_base {
public:
    explicit asm_data_listener(
        std::shared_ptr<dds::service_config> service_config)
        : product_listener_base(std::move(service_config)){};
    void on_update(const config &config) override;
    void on_unapply(const config & /*config*/) override{};

protected:
    static void extract_data(rapidjson::Value::ConstMemberIterator itr,
        dds::service_config::rule_data &rule_data);
};

} // namespace dds::remote_config
