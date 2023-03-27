// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "engine.hpp"
#include "exception.hpp"
#include "remote_config/client.hpp"
#include "remote_config/settings.hpp"
#include "scheduler.hpp"
#include "service_config.hpp"
#include "service_identifier.hpp"
#include "std_logging.hpp"
#include "utils.hpp"
#include <future>
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace dds {

using namespace std::chrono_literals;

class service {
public:
    using ptr = std::shared_ptr<service>;

    service(service_identifier id, std::shared_ptr<engine> engine,
        remote_config::client::ptr &&rc_client,
        std::shared_ptr<service_config> service_config,
        dds::scheduler &&scheduler);

    static service::ptr from_settings(const service_identifier &id,
        const dds::engine_settings &eng_settings,
        const remote_config::settings &rc_settings,
        std::map<std::string_view, std::string> &meta,
        std::map<std::string_view, double> &metrics, bool dynamic_enablement);

    [[nodiscard]] std::shared_ptr<engine> get_engine() const
    {
        // TODO make access atomic?
        return engine_;
    }

    [[nodiscard]] std::shared_ptr<service_config> get_service_config() const
    {
        // TODO make access atomic?
        return service_config_;
    }

    [[nodiscard]] service_identifier get_id() const { return id_; }
    bool run();
protected:
    void handle_error();

    service_identifier id_;
    std::shared_ptr<engine> engine_;
    remote_config::client::ptr rc_client_;
    std::shared_ptr<service_config> service_config_;
    dds::scheduler scheduler_;

    bool poll();
    bool discover();
    std::function<bool()> rc_action_;
};

} // namespace dds
