// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include "scheduler.hpp"

namespace dds {

namespace mock {
class scheduler : public dds::scheduler {
public:
    scheduler(const std::chrono::milliseconds &poll_interval,
        const std::chrono::milliseconds &max_time_interval)
        : dds::scheduler(poll_interval, max_time_interval){};
    auto get_interval() { return interval_; }
    auto get_poll_interval() { return poll_interval_; }
    auto get_max_allowed_interval() { return max_allowed_interval_; }
    auto get_errors() { return errors_; }
    void set_errors(std::uint16_t errors) { errors_ = errors; }
    void tick(scheduler::action *action) override
    {
        dds::scheduler::tick(action);
    };
};

class action : public dds::scheduler::action {
public:
    MOCK_METHOD(bool, act, (), (override));
};
} // namespace mock

ACTION_P(SignalCall, promise) { promise->set_value(true); }

TEST(SchedulerTest, ItCreatesSchedulerUsingIntervalsGiven)
{
    mock::scheduler scheduler(1s, 2s);

    EXPECT_EQ(1s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(2s, scheduler.get_max_allowed_interval());
}

TEST(SchedulerTest, OnErrorIncreasesIntervals)
{
    mock::scheduler scheduler(1s, 30s);
    mock::action action;

    EXPECT_CALL(action, act()).WillRepeatedly(Return(false));

    scheduler.tick(&action);

    EXPECT_EQ(2s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(1, scheduler.get_errors());

    scheduler.tick(&action);

    EXPECT_EQ(4s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(2, scheduler.get_errors());
}

TEST(SchedulerTest, WhenActionReturnsTrueItResetIntervalsAnErrors)
{
    mock::scheduler scheduler(1s, 30s);
    mock::action action;

    EXPECT_CALL(action, act()).WillOnce(Return(false));
    scheduler.tick(&action);

    EXPECT_EQ(2s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(1, scheduler.get_errors());

    EXPECT_CALL(action, act()).WillOnce(Return(true));
    scheduler.tick(&action);

    EXPECT_EQ(1s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(0, scheduler.get_errors());
}

TEST(SchedulerTest, ItCapsIntervalToSpecifiedOnConstructor)
{
    mock::scheduler scheduler(1s, 3s);
    mock::action action;

    EXPECT_CALL(action, act()).WillRepeatedly(Return(false));

    scheduler.tick(&action);

    EXPECT_EQ(2s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(1, scheduler.get_errors());

    scheduler.tick(&action);

    EXPECT_EQ(3s, scheduler.get_interval());
    EXPECT_EQ(1s, scheduler.get_poll_interval());
    EXPECT_EQ(2, scheduler.get_errors());
}

TEST(SchedulerTest, ItAllowsIntevalsHigherThanMaxButDoesNotIncreaseThen)
{
    mock::scheduler scheduler(10s, 3s);
    mock::action action;

    EXPECT_CALL(action, act()).WillRepeatedly(Return(false));

    scheduler.tick(&action);

    EXPECT_EQ(10s, scheduler.get_interval());
    EXPECT_EQ(10s, scheduler.get_poll_interval());
    EXPECT_EQ(1, scheduler.get_errors());

    scheduler.tick(&action);

    EXPECT_EQ(10s, scheduler.get_interval());
    EXPECT_EQ(10s, scheduler.get_poll_interval());
    EXPECT_EQ(2, scheduler.get_errors());
}

TEST(SchedulerTest, ItDoesNotOverflowErrors)
{
    mock::scheduler scheduler(1s, 3s);
    mock::action action;

    scheduler.set_errors(std::numeric_limits<std::uint16_t>::max());

    EXPECT_CALL(action, act()).WillRepeatedly(Return(false));
    scheduler.tick(&action);
    scheduler.tick(&action);
    scheduler.tick(&action);
    scheduler.tick(&action);
    scheduler.tick(&action);

    EXPECT_EQ(
        std::numeric_limits<std::uint16_t>::max(), scheduler.get_errors());
}

TEST(SchedulerTest, ValidateSchedulerThread)
{
    { // It has only time for one call
        mock::scheduler scheduler(100ms, 3s);
        mock::action action;

        std::promise<bool> call_promise;
        // Only one call on 10ms
        EXPECT_CALL(action, act()).Times(1).WillRepeatedly(DoAll(Return(true)));
        scheduler.start(&action);
        call_promise.get_future().wait_for(10ms);
    }
    { // It has only time for two calls
        mock::scheduler scheduler(100ms, 3s);
        mock::action action;

        std::promise<bool> call_promise;
        // Two calls on 110ms
        EXPECT_CALL(action, act()).Times(2).WillRepeatedly(DoAll(Return(true)));
        scheduler.start(&action);
        call_promise.get_future().wait_for(110ms);
    }
}
} // namespace dds
