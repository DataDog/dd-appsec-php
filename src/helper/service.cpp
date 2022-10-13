// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "service.hpp"

namespace dds {

service::ptr service::from_settings(const identifier &id,
    const dds::client_settings &eng_settings,
    /*remote_config::settings &rc_settings,*/
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

    return std::make_shared<service>(id, engine_ptr);
}

} // namespace dds
