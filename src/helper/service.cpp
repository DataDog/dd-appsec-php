// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service.hpp"

namespace dds {

service::service(service_identifier id, std::shared_ptr<engine> &engine,
    remote_config::client::ptr &&rc_client,
    const std::chrono::milliseconds &poll_interval)
    : id_(std::move(id)), engine_(std::move(engine)),
      rc_client_(std::move(rc_client)), poll_interval_(poll_interval)
{
    // The engine should always be valid
    // TODO: use a meaninful exception
    if (!engine_) {
        throw;
    }

    if (rc_client_) {
        // TODO: move this thread to an abstraction within remote_config
        running_ = true;
        handler_ = std::thread(&service::run, this);
    }
}

service::~service()
{
    if (running_) {
        running_ = false;
        handler_.join();
    }
}

service::ptr service::from_settings(const service_identifier &id,
    const dds::engine_settings &eng_settings,
    const remote_config::settings &rc_settings,
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics)
{
    // no cache hit
    auto &&rules_path = eng_settings.rules_file_or_default();
    std::shared_ptr engine_ptr{engine::create(eng_settings.trace_rate_limit)};

    try {
        SPDLOG_DEBUG("Will load WAF rules from {}", rules_path);
        // may throw std::exception
        const subscriber::ptr waf =
            waf::instance::from_settings(eng_settings, meta, metrics);
        engine_ptr->subscribe(waf);
    } catch (...) {
        DD_STDLOG(DD_STDLOG_WAF_INIT_FAILED, rules_path);
        throw;
    }

    std::chrono::milliseconds poll_interval{rc_settings.poll_interval};
    auto rc_client = remote_config::client::from_settings(id, rc_settings);
    return std::make_shared<service>(id, engine_ptr, std::move(rc_client),
        std::chrono::milliseconds{rc_settings.poll_interval});
}

void service::run()
{
    while (running_) {
        if (!rc_client_->poll()) {
            return;
        }

        std::this_thread::sleep_for(poll_interval_);
    }
}
} // namespace dds
