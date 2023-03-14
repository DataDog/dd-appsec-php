// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service.hpp"

namespace dds {

service::service(service_identifier id, std::shared_ptr<engine> engine,
    remote_config::client::ptr &&rc_client,
    std::shared_ptr<service_config> service_config, dds::scheduler &&scheduler)
    : id_(std::move(id)), engine_(std::move(engine)),
      service_config_(std::move(service_config)),
      rc_client_(std::move(rc_client)), scheduler_(std::move(scheduler))
{
    // It starts checking if rc is available
    rc_action_ = [this]() -> bool { return discover(); };
    // The engine should always be valid
    if (!engine_) {
        throw std::runtime_error("invalid engine");
    }

    if (rc_client_) {
        scheduler_.start([this]() -> bool { return run(); });
    }
}

service::ptr service::from_settings(const service_identifier &id,
    const dds::engine_settings &eng_settings,
    const remote_config::settings &rc_settings,
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics, bool dynamic_enablement)
{
    auto engine_ptr = engine::from_settings(eng_settings, meta, metrics);

    std::chrono::milliseconds const poll_interval{rc_settings.poll_interval};
    auto service_config = std::make_shared<dds::service_config>();
    service_config->dynamic_enablement = dynamic_enablement;
    service_config->dynamic_engine = eng_settings.rules_file.empty();

    auto rc_client = remote_config::client::from_settings(
        id, rc_settings, service_config, engine_ptr);

    dds::scheduler scheduler(poll_interval,
        std::max(
            std::chrono::milliseconds(std::chrono::minutes(5)), poll_interval));

    return std::make_shared<service>(id, engine_ptr, std::move(rc_client),
        std::move(service_config), std::move(scheduler));
}

void service::handle_error()
{
    std::cout << "handling error" << std::endl;

    rc_action_ = [this]() -> bool { return discover(); };
}

bool service::poll()
{
    std::cout << "poll" << std::endl;

    try {
        rc_client_->poll();
        return true;
    } catch (dds::remote_config::network_exception & /** e */) {
        handle_error();
    }
    return false;
}
bool service::discover()
{
    std::cout << "discover" << std::endl;
    try {
        if (rc_client_->is_remote_config_available()) {
            // Remote config is available. Start polls
            rc_action_ = [this]() -> bool { return poll(); };
            std::cout << "discover true" << std::endl;

            return true;
        }
        std::cout << "discover false" << std::endl;
    } catch (dds::remote_config::network_exception & /** e */) {}
    std::cout << "discover end" << std::endl;
    handle_error();
    return false;
}

bool service::run() {
    std::cout << "Run" << std::endl;
    return rc_action_();
}
} // namespace dds
