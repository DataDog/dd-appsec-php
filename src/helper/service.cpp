// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service.hpp"

namespace dds {

service::service(service_identifier id, std::shared_ptr<engine> &engine,
    remote_config::client::ptr &&rc_client):
  id_(std::move(id)), engine_(std::move(engine)),
  rc_client_(std::move(rc_client))
{
    // The engine should always be valid
    // TODO: use a meaninful exception
    if (!engine_) {
        throw;
    }

    if (rc_client_) {
        running_ = true;
        handler_ = std::thread([this]() mutable {
            using namespace std::chrono_literals;
            while (this->running_) {
                this->rc_client_->poll();
                std::this_thread::sleep_for(1s);
            }
        });
    }
}

service::~service() {
    if (running_) {
        running_ = false;
        handler_.join();
    }
}

service::ptr service::from_settings(
    const service_identifier &id,
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
        const subscriber::ptr waf = waf::instance::from_settings(
            eng_settings, meta, metrics);
        engine_ptr->subscribe(waf);
    } catch (...) {
        DD_STDLOG(DD_STDLOG_WAF_INIT_FAILED, rules_path);
        throw;
    }

    auto rc_client = remote_config::client::from_settings(id, rc_settings);
    return std::make_shared<service>(id, engine_ptr, std::move(rc_client));
}

} // namespace dds
