// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "engine.hpp"
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

namespace dds::remote_config {

using namespace std::chrono_literals;

class client_handler : public scheduler::action {
public:
    using ptr = std::shared_ptr<client_handler>;

    client_handler(remote_config::client::ptr &&rc_client,
        std::shared_ptr<service_config> service_config,
        dds::scheduler &&scheduler);

    client_handler(const client_handler &) = delete;
    client_handler &operator=(const client_handler &) = delete;

    client_handler(client_handler &&) = delete;
    client_handler &operator=(client_handler &&) = delete;

    static client_handler::ptr from_settings(service_identifier &&id,
        const dds::engine_settings &eng_settings,
        std::shared_ptr<dds::service_config> service_config,
        const remote_config::settings &rc_settings,
        const engine::ptr &engine_ptr, bool dynamic_enablement);

    bool start();

    bool act();

    remote_config::client *get_client() { return rc_client_.get(); }

protected:
    void run(std::future<bool> &&exit_signal);
    void handle_error();

    remote_config::client::ptr rc_client_;
    std::shared_ptr<service_config> service_config_;
    dds::scheduler scheduler_;
    bool poll();
    bool discover();
    std::function<bool()> rc_action_;

    std::uint16_t errors_ = {0};
};

} // namespace dds::remote_config
