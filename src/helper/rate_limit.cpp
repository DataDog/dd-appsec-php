// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#include "rate_limit.hpp"

#include <chrono>
#include <iostream>
#include <limits>
#include <tuple>

using std::chrono::duration_cast;
using std::chrono::microseconds;

using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::system_clock;

namespace dds {

namespace {
auto get_time()
{
    auto now = system_clock::now().time_since_epoch();
    return std::make_tuple(duration_cast<milliseconds>(now).count(),
        duration_cast<seconds>(now).count());
}
} // namespace

rate_limiter::rate_limiter(uint32_t max_per_second, milliseconds heartbeat_rate)
    : max_per_second_(max_per_second),
      heartbeat_rate_(duration_cast<milliseconds>(heartbeat_rate).count()),
      last_heartbeat_(0)
{}

bool rate_limiter::allow()
{
    if (max_per_second_ == 0) {
        return true;
    }

    auto [now_ms, now_s] = get_time();

    std::lock_guard<std::mutex> const lock(mtx_);

    if (now_s != index_s_) {
        if (index_s_ == now_s - 1) {
            precounter_s_ = counter_s_;
        } else {
            precounter_s_ = 0;
        }
        counter_s_ = 0;
        index_s_ = now_s;
    }

    constexpr uint64_t mil = 1000;
    uint32_t const count =
        (precounter_s_ * (mil - (now_ms % mil))) / mil + counter_s_;

    if (count >= max_per_second_) {
        return false;
    }

    counter_s_++;
    last_heartbeat_ = now_ms;

    return true;
}
bool rate_limiter::heartbeat()
{
    if (heartbeat_rate_ == 0) {
        return false;
    }

    auto [now_ms, _] = get_time();

    std::lock_guard<std::mutex> const lock(mtx_);

    if (now_ms - last_heartbeat_ < heartbeat_rate_) {
        return false;
    }

    last_heartbeat_ = now_ms;

    return true;
}

} // namespace dds
