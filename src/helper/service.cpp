// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service.hpp"

namespace dds {

service::service(service_identifier id, std::shared_ptr<engine> engine,
    std::shared_ptr<service_config> service_config,
    dds::remote_config::service_handler::ptr service_handler)
    : id_(std::move(id)), engine_(std::move(engine)),
      service_config_(std::move(service_config)),
      service_handler_(service_handler)
{
    // The engine should always be valid
    if (!engine_) {
        throw std::runtime_error("invalid engine");
    }

    if (service_handler_) {
        service_handler_->start();
    }
}

service::ptr service::from_settings(const service_identifier &id,
    const dds::engine_settings &eng_settings,
    const remote_config::settings &rc_settings,
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics, bool dynamic_enablement)
{
    auto engine_ptr = engine::from_settings(eng_settings, meta, metrics);

    auto service_config = std::make_shared<dds::service_config>();

    auto service_handler =
        remote_config::service_handler::from_settings(id, eng_settings,
            service_config, rc_settings, engine_ptr, dynamic_enablement);

    return std::make_shared<service>(
        id, engine_ptr, std::move(service_config), std::move(service_handler));
}
} // namespace dds
