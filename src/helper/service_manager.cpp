// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "service_manager.hpp"

namespace dds {

std::shared_ptr<service> service_manager::create_service(
    const service_identifier &id, const engine_settings &settings,
    const remote_config::settings &rc_settings,
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics)
{
    const std::lock_guard guard{mutex_};

    auto hit = cache_.find(id);
    if (hit != cache_.end()) {
        auto service_ptr = hit->second.lock();
        if (service_ptr) { // not expired
            return service_ptr;
        }
    }

    service_identifier new_id = id;
    // If extension did not send runtime_id, then it needs to be generated here
    if (new_id.runtime_id.empty()) {
        new_id.runtime_id = generate_random_uuid();
    }

    auto service_ptr =
        service::from_settings(new_id, settings, rc_settings, meta, metrics);
    cache_.emplace(new_id, std::move(service_ptr));
    last_service_ = service_ptr;

    cleanup_cache();

    return service_ptr;
}

void service_manager::cleanup_cache()
{
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->second.expired()) {
            it = cache_.erase(it);
        } else {
            it++;
        }
    }
}

} // namespace dds
