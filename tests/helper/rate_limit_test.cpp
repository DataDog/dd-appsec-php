// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include "rate_limit.hpp"
#include <iostream>
#include <thread>

using std::chrono::milliseconds;

namespace dds {

TEST(RateLimiter, HeartBeatIsFalseIfRateIsZero)
{
    milliseconds heartbeat_rate = 0s;
    rate_limiter limiter(123, heartbeat_rate);
    EXPECT_FALSE(limiter.heartbeat());
}

TEST(RateLimiter, HeartBeat)
{
    milliseconds heartbeat_rate = 200ms;
    rate_limiter limiter(123, heartbeat_rate);

    EXPECT_TRUE(limiter.heartbeat());
    EXPECT_FALSE(limiter.heartbeat());

    std::this_thread::sleep_for(heartbeat_rate);

    EXPECT_TRUE(limiter.heartbeat());
    EXPECT_FALSE(limiter.heartbeat());
    EXPECT_FALSE(limiter.heartbeat());
}


TEST(RateLimiter, HeartBeatTakesIntoAccountOtherCalls)
{
    milliseconds heartbeat_rate = 100ms;
    rate_limiter limiter(123, heartbeat_rate);

    EXPECT_TRUE(limiter.allow());
    EXPECT_FALSE(limiter.heartbeat());

    std::this_thread::sleep_for(heartbeat_rate);

    EXPECT_TRUE(limiter.allow());
    EXPECT_FALSE(limiter.heartbeat());
    EXPECT_FALSE(limiter.heartbeat());
}

} // namespace dds
