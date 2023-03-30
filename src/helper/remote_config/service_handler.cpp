// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service_handler.hpp"
#include "remote_config/asm_data_listener.hpp"
#include "remote_config/asm_dd_listener.hpp"
#include "remote_config/asm_features_listener.hpp"
#include "remote_config/asm_listener.hpp"

namespace dds::remote_config {

// This will limit the max increase to 4.266666667 minutes
static constexpr std::uint16_t max_increment = 8;

service_handler::service_handler(remote_config::client::ptr &&rc_client,
    std::shared_ptr<service_config> service_config,
    const std::chrono::milliseconds &poll_interval)
    : service_config_(std::move(service_config)),
      rc_client_(std::move(rc_client)), poll_interval_(poll_interval),
      interval_(poll_interval)
{
    // It starts checking if rc is available
    rc_action_ = [this] { discover(); };
}

service_handler::~service_handler()
{
    if (handler_.joinable()) {
        exit_.set_value(true);
        handler_.join();
    }
}

service_handler::ptr service_handler::from_settings(
    const dds::engine_settings &eng_settings,
    std::shared_ptr<dds::service_config> service_config,
    const remote_config::settings &rc_settings, engine::ptr engine_ptr,
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
    if (service_config->sid.runtime_id.empty()) {
        service_config->sid.runtime_id = generate_random_uuid();
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
        service_config->sid, rc_settings, products);

    return std::make_shared<service_handler>(std::move(rc_client),
        std::move(service_config),
        std::chrono::milliseconds{rc_settings.poll_interval});
}

bool service_handler::start()
{
    if (rc_client_) {
        handler_ = std::thread(&service_handler::run, this, exit_.get_future());
        return true;
    }

    return false;
}

void service_handler::handle_error()
{
    rc_action_ = [this] { discover(); };
    interval_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        poll_interval_ * pow(2, std::min(errors_, max_increment)));
    if (errors_ < std::numeric_limits<std::uint16_t>::max() - 1) {
        errors_++;
    }
}

void service_handler::poll()
{
    try {
        rc_client_->poll();
    } catch (dds::remote_config::network_exception & /** e */) {
        handle_error();
    }
}
void service_handler::discover()
{
    try {
        if (rc_client_->is_remote_config_available()) {
            // Remote config is available. Start polls
            rc_action_ = [this] { poll(); };
            errors_ = 0;
            interval_ = poll_interval_;
            return;
        }
    } catch (dds::remote_config::network_exception & /** e */) {}
    handle_error();
}

void service_handler::run(std::future<bool> &&exit_signal)
{
    std::chrono::time_point<std::chrono::steady_clock> before{0s};
    std::future_status fs = exit_signal.wait_for(0s);
    while (fs == std::future_status::timeout) {
        // If the thread is interrupted somehow, make sure to check that
        // the polling interval has actually elapsed.
        auto now = std::chrono::steady_clock::now();
        if ((now - before) >= interval_) {
            rc_action_();
            before = now;
        }

        fs = exit_signal.wait_for(interval_);
    }
}
} // namespace dds::remote_config