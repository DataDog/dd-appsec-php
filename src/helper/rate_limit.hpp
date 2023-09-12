// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#pragma once

#include <atomic>
#include <mutex>
using std::chrono::milliseconds;

namespace dds {

class rate_limiter {
public:
    explicit /**/ rate_limiter(
        uint32_t max_per_second, milliseconds heartbeat_rate);
    bool allow();

    bool heartbeat();

protected:
    std::mutex mtx_;
    uint32_t index_s_{0};
    uint32_t counter_s_{0};
    uint32_t precounter_s_{0};
    const uint32_t max_per_second_;
    const uint32_t heartbeat_rate_;
    uint64_t last_heartbeat_;
};

} // namespace dds
