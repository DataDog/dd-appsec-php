// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#include <iostream>
#include <mutex>

#pragma once

namespace dds {
class sampler {
public:
    sampler(bool enabled, double sample_rate) : enabled_(enabled)
    {
        if (sample_rate < 0 || sample_rate > 1) {
            sample_rate = default_sample_rate;
        }

        tokens_per_hundred_ = (unsigned)round(sample_rate * 100);
        reset_tokens();
    }
    class scope {
    public:
        scope(std::atomic<bool> &concurrent) : concurrent_(&concurrent)
        {
            *concurrent_ = true;
        }

        scope(const scope &) = delete;
        scope &operator=(const scope &) = delete;
        scope(scope &&oth)
        {
            concurrent_ = oth.concurrent_;
            oth.concurrent_ = nullptr;
        }
        scope &operator=(scope &&oth)
        {
            concurrent_ = oth.concurrent_;
            oth.concurrent_ = nullptr;

            return *this;
        }

        ~scope()
        {
            if (concurrent_ != nullptr) {
                *concurrent_ = false;
            }
        }

    protected:
        std::atomic<bool> *concurrent_;
    };

    std::optional<scope> get()
    {
        if (!enabled_) {
            return std::nullopt;
        }
        const std::lock_guard<std::mutex> lock_guard(mtx_);
        if (request_++ == 100) {
            reset_tokens();
            request_ = 0;
        }
        if (tokens_available_ > 0 && !concurrent_) {
            tokens_available_--;
            return {scope{concurrent_}};
        }
        return std::nullopt;
    }

protected:
    static constexpr double default_sample_rate = 0.1; // 10% of requests
    void reset_tokens() { tokens_available_ = tokens_per_hundred_; }
    unsigned request_{0};
    std::atomic<bool> concurrent_{false};
    std::mutex mtx_;
    const bool enabled_;
    unsigned tokens_per_hundred_;
    unsigned tokens_available_;
};
} // namespace dds
