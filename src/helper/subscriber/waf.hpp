// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <chrono>
#include <ddwaf.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

#include "../engine.hpp"
#include "../exception.hpp"
#include "../parameter.hpp"

namespace dds::waf {

void initialise_logging(spdlog::level::level_enum level);

class instance : public dds::subscriber {
public:
    static constexpr int default_waf_timeout_us = 10000;

    using ptr = std::shared_ptr<instance>;
    class listener : public dds::subscriber::listener {
    public:
        listener(ddwaf_context ctx, std::chrono::microseconds waf_timeout);
        listener(const listener &) = delete;
        listener &operator=(const listener &) = delete;
        listener(listener &&) noexcept;
        listener &operator=(listener &&) noexcept;
        ~listener() override;

        dds::result call(dds::parameter_view &data,
            std::map<std::string, double> &metrics) override;

    protected:
        ddwaf_context handle_{};
        std::chrono::microseconds waf_timeout_;
    };

    // NOLINTNEXTLINE(google-runtime-references)
    instance(dds::parameter &rule, 
        std::map<std::string, std::string> &meta,
        std::map<std::string, double> &metrics,
        std::uint64_t waf_timeout_us);
    instance(const instance &) = delete;
    instance &operator=(const instance &) = delete;
    instance(instance &&) noexcept;
    instance &operator=(instance &&) noexcept;
    ~instance() override;

    std::vector<std::string_view> get_subscriptions() override;

    listener::ptr get_listener() override;

    static ptr from_settings(
        const client_settings &settings,
        std::map<std::string, std::string> &meta,
        std::map<std::string, double> &metrics);

    // testing only
    static instance::ptr from_string(
        std::string_view rule, 
        std::map<std::string, std::string> &meta,
        std::map<std::string, double> &metrics,
        std::uint64_t waf_timeout_us = default_waf_timeout_us);

protected:
    ddwaf_handle handle_{nullptr};
    std::chrono::microseconds waf_timeout_;
};

parameter parse_file(std::string_view filename);
parameter parse_string(std::string_view config);

} // namespace dds::waf
