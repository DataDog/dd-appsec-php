// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "scheduler.hpp"

namespace dds {

bool scheduler::is_time()
{
    auto now = std::chrono::steady_clock::now();
    if ((now - before) >= interval_) {
        before = now;
        return true;
    }

    return false;
}

scheduler::~scheduler()
{
    if (handler_.joinable()) {
        exit_.set_value(true);
        handler_.join();
    }
}

void scheduler::error()
{
    if (interval_ < max_allowed_interval_) {
        auto new_interval =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                poll_interval_ * pow(2, errors_));
        interval_ = std::min(max_allowed_interval_, new_interval);
    }
    if (errors_ < std::numeric_limits<std::uint16_t>::max() - 1) {
        errors_++;
    }
}

void scheduler::start(scheduler::action *action)
{
    handler_ = std::thread(&scheduler::run, this, exit_.get_future(), action);
}

void scheduler::run(std::future<bool> &&exit_signal, scheduler::action *action)
{
    std::future_status fs = exit_signal.wait_for(0s);
    while (fs == std::future_status::timeout) {
        // If the thread is interrupted somehow, make sure to check that
        // the polling interval has actually elapsed.
        if (is_time()) {
            if (!action->act()) {
                error();
            } else {
                reset();
            }
        }

        fs = exit_signal.wait_for(interval_);
    }
}
} // namespace dds