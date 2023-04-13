// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <future>
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace dds {

using namespace std::chrono_literals;

class scheduler {
public:
    class action {
    public:
        virtual bool act() = 0;
    };
    scheduler(const std::chrono::milliseconds &poll_interval,
        const std::chrono::milliseconds &max_time_interval)
        : poll_interval_(poll_interval), interval_(poll_interval),
          max_allowed_interval_(max_time_interval)
    {}
    ~scheduler();
    scheduler(const scheduler &) = delete;
    scheduler &operator=(const scheduler &) = delete;

    scheduler(scheduler &&) = default;
    scheduler &operator=(scheduler &&) = delete;

    virtual void start(scheduler::action *action);
    void run(std::future<bool> &&exit_signal, scheduler::action *action);

protected:
    bool is_time();
    void error();
    virtual void tick(scheduler::action *action);
    void reset()
    {
        errors_ = 0;
        interval_ = poll_interval_;
    }

    std::chrono::milliseconds poll_interval_;
    std::chrono::milliseconds interval_;
    std::chrono::milliseconds max_allowed_interval_;
    std::uint16_t errors_ = {0};
    std::chrono::time_point<std::chrono::steady_clock> before{0s};

    std::promise<bool> exit_;
    std::thread handler_;
};

} // namespace dds