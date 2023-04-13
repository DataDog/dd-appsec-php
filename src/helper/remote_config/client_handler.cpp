// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "client_handler.hpp"
#include "remote_config/asm_data_listener.hpp"
#include "remote_config/asm_dd_listener.hpp"
#include "remote_config/asm_features_listener.hpp"
#include "remote_config/asm_listener.hpp"

namespace dds::remote_config {

client_handler::client_handler(remote_config::client::ptr &&rc_client,
    std::shared_ptr<service_config> service_config, dds::scheduler &&scheduler)
    : service_config_(std::move(service_config)),
      rc_client_(std::move(rc_client)), scheduler_(std::move(scheduler))
{
    // It starts checking if rc is available
    rc_action_ = [this]() -> bool { return discover(); };
}

client_handler::ptr client_handler::from_settings(service_identifier &&id,
    const dds::engine_settings &eng_settings,
    std::shared_ptr<dds::service_config> service_config,
    const remote_config::settings &rc_settings, const engine::ptr &engine_ptr,
    bool dynamic_enablement)
{
    if (!rc_settings.enabled) {
        return {};
    }

    if (!service_config) {
        return {};
    }

    // TODO runtime_id will be send by the extension when the extension can get
    // it from the profiler. When that happen, this wont be needed
    if (id.runtime_id.empty()) {
        id.runtime_id = generate_random_uuid();
    }

    std::vector<remote_config::product> products = {};
    if (dynamic_enablement) {
        auto asm_features_listener =
            std::make_shared<remote_config::asm_features_listener>(
                service_config);
        products.emplace_back(asm_features_listener);
    }
    if (eng_settings.rules_file.empty()) {
        auto asm_data_listener =
            std::make_shared<remote_config::asm_data_listener>(engine_ptr);
        auto asm_dd_listener = std::make_shared<remote_config::asm_dd_listener>(
            engine_ptr, dds::engine_settings::default_rules_file());
        auto asm_listener =
            std::make_shared<remote_config::asm_listener>(engine_ptr);

        products.emplace_back(asm_data_listener);
        products.emplace_back(asm_dd_listener);
        products.emplace_back(asm_listener);
    }

    if (products.empty()) {
        return {};
    }

    auto rc_client = remote_config::client::from_settings(
        std::move(id), remote_config::settings(rc_settings), products);

    auto scheduler = dds::scheduler(1s, 5min);

    return std::make_shared<client_handler>(
        std::move(rc_client), std::move(service_config), std::move(scheduler));
}

bool client_handler::start()
{
    if (rc_client_) {
        scheduler_.start(this);
        return true;
    }

    return false;
}

bool client_handler::act() { return rc_action_(); }

void client_handler::handle_error()
{
    rc_action_ = [this]() -> bool { return discover(); };
}

bool client_handler::poll()
{
    try {
        if (rc_client_->poll()) {
            return true;
        }
    } catch (dds::remote_config::network_exception & /** e */) {}
    handle_error();

    return false;
}
bool client_handler::discover()
{
    try {
        if (rc_client_->is_remote_config_available()) {
            // Remote config is available. Start polls
            rc_action_ = [this]() -> bool { return poll(); };
            return true;
        }
    } catch (dds::remote_config::network_exception & /** e */) {}
    handle_error();

    return false;
}

} // namespace dds::remote_config
