// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "../engine.hpp"
#include "config.hpp"
#include "listener.hpp"
#include "parameter.hpp"
#include <optional>
#include <rapidjson/document.h>
#include <utility>

namespace dds::remote_config {

class asm_listener : public product_listener_base {
public:
    explicit asm_listener(std::shared_ptr<dds::engine> engine)
        : engine_(std::move(engine)){};
    void on_update(const config &config) override;
    void on_unapply(const config & /*config*/) override{};
    remote_config::protocol::capabilities_e get_capabilities() override
    {
        return remote_config::protocol::capabilities_e::ASM_EXCLUSIONS |
               remote_config::protocol::capabilities_e::
                   ASM_CUSTOM_BLOCKING_RESPONSE |
               remote_config::protocol::capabilities_e::ASM_REQUEST_BLOCKING |
               remote_config::protocol::capabilities_e::ASM_RESPONSE_BLOCKING;
    }

    void init() override;
    void commit() override;

protected:
    std::shared_ptr<dds::engine> engine_;
    rapidjson::Document ruleset_;
};

} // namespace dds::remote_config
