// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#pragma once

#include <atomic>
#include <cmath>
#include <iostream>
#include <mutex>
#include <optional>

namespace dds {
class sampler {
public:
    sampler(double sample_rate)
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

        if (sample_rate < 0 || sample_rate > 1 ||
            (sample_rate > 0 && sample_rate < 0.0001)) {
            sample_rate = default_sample_rate;
        }
        double tmp = sample_rate;
        do {
            tmp = tmp * 10;
            group_size_ = group_size_ * 10;
            // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        } while (sample_rate > 0 && tmp < 1);

        tokens_per_group_ = (unsigned)round(sample_rate * group_size_);
        reset_tokens();
    }
    class scope {
    public:
        explicit scope(std::atomic<bool> &concurrent) : concurrent_(&concurrent)
        {
            *concurrent_ = true;
        }

        scope(const scope &) = delete;
        scope &operator=(const scope &) = delete;
        scope(scope &&oth) noexcept
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
        const std::lock_guard<std::mutex> lock_guard(mtx_);
        if (request_++ == group_size_) {
            reset_tokens();
            request_ = 1;
        }
        if (tokens_available_ > 0 && !concurrent_) {
            tokens_available_--;
            return {scope{concurrent_}};
        }
        return std::nullopt;
    }

protected:
    static constexpr double default_sample_rate = 0.1; // 10% of requests
    void reset_tokens() { tokens_available_ = tokens_per_group_; }
    unsigned request_{0};
    std::atomic<bool> concurrent_{false};
    std::mutex mtx_;
    unsigned tokens_per_group_;
    unsigned group_size_{1};
    unsigned tokens_available_;
};
} // namespace dds
