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

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::duration_cast;
using std::chrono::system_clock;

namespace dds {

namespace {
auto get_time() {
    auto now = system_clock::now().time_since_epoch();
    return std::make_tuple(duration_cast<milliseconds>(now).count(),
                           duration_cast<seconds>(now).count());
}
}

rate_limiter::rate_limiter(unsigned max_per_second) :
    max_per_second_(max_per_second) {}

bool rate_limiter::allow()
{
    if (max_per_second_ == 0) { return true; }

    auto [now_ms, now_s] = get_time();

    std::lock_guard<std::mutex> lock(mtx_);

    if (now_s != index_) {
        if (index_ == now_s - 1) {
            precounter_ = counter_;
        } else {
            precounter_ = 0;
        }
        counter_ = 0;
        index_ = now_s;
    }

    uint32_t count = (precounter_ * (1000 - (now_ms % 1000)))/1000 + counter_;

    if (count >= max_per_second_) { return false; }

    counter_++;

    return true;
}

}
