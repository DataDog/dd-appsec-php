// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include <sampler.hpp>
#include <thread>

namespace dds {

std::atomic<int> picked = 0;

void count_picked(dds::sampler &sampler, int iterations)
{
    for (int i = 0; i < iterations; i++) {
        auto is_pick = sampler.get();
        if (is_pick != std::nullopt) {
            picked++;
        }
    }
}

int run_sample(dds::sampler &sampler)
{
    picked = 0;

    std::thread first(count_picked, std::ref(sampler), 50);
    std::thread second(count_picked, std::ref(sampler), 50);

    first.join();
    second.join();

    return picked;
}

TEST(SamplerTest, DisabledDoesNotPickAny)
{
    sampler s(false, 1);

    EXPECT_EQ(0, run_sample(s));
}

TEST(SamplerTest, ItPicksAllWhenRateIs1)
{
    sampler s(true, 1);

    EXPECT_EQ(100, run_sample(s));
}

TEST(SamplerTest, ItPicksNoneWhenRateIs0)
{
    sampler s(true, 0);

    EXPECT_EQ(0, run_sample(s));
}

TEST(SamplerTest, ItPicksHalfWhenPortionGiven)
{
    sampler s(true, 0.5);

    EXPECT_EQ(50, run_sample(s));
}

TEST(SamplerTest, ItResetTokensAfter100Calls)
{
    sampler s(true, 1);

    picked = 0;
    count_picked(s, 100);
    count_picked(s, 100);

    EXPECT_EQ(200, picked);
}
} // namespace dds
