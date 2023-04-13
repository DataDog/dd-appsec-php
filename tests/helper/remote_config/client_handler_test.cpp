// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "../common.hpp"
#include "remote_config/client_handler.hpp"

namespace dds {

namespace mock {
class client : public remote_config::client {
public:
    client(service_identifier &&sid)
        : remote_config::client(nullptr, std::move(sid), {})
    {}
    ~client() = default;
    MOCK_METHOD0(poll, bool());
    MOCK_METHOD0(is_remote_config_available, bool());
};

class scheduler : public dds::scheduler {
public:
    scheduler(const std::chrono::milliseconds &poll_interval,
        const std::chrono::milliseconds &max_time_interval)
        : dds::scheduler(poll_interval, max_time_interval){};
    MOCK_METHOD(void, start, (scheduler::action * action), (override));
};

} // namespace mock

ACTION_P(SignalCall, promise) { promise->set_value(true); }

class ClientHandlerTest : public ::testing::Test {
public:
    service_identifier sid{
        "service", "env", "tracer_version", "app_version", "runtime_id"};
    dds::engine_settings settings;
    remote_config::settings rc_settings;
    std::shared_ptr<dds::service_config> service_config;
    service_identifier id;
    std::shared_ptr<dds::engine> engine;
    dds::scheduler scheduler{500ms, 5min};

    void SetUp()
    {
        service_config = std::make_shared<dds::service_config>();
        id = sid;
        engine = engine::create();
        rc_settings.enabled = true;
    }
};

TEST_F(ClientHandlerTest, IfRemoteConfigDisabledItDoesNotGenerateHandler)
{
    rc_settings.enabled = false;

    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, false);

    EXPECT_FALSE(client_handler);
}

TEST_F(ClientHandlerTest, IfNoServiceConfigProvidedItDoesNotGenerateHandler)
{
    std::shared_ptr<dds::service_config> null_service_config = {};
    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, null_service_config, rc_settings,
        engine, false);

    EXPECT_FALSE(client_handler);
}

TEST_F(ClientHandlerTest, RuntimeIdIsNotGeneratedIfProvided)
{
    const char *runtime_id = "some runtime id";
    id.runtime_id = runtime_id;

    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, false);

    EXPECT_STREQ(runtime_id, client_handler->get_client()
                                 ->get_service_identifier()
                                 .runtime_id.c_str());
}

TEST_F(ClientHandlerTest, RuntimeIdIsGeneratedWhenNotProvided)
{
    id.runtime_id.clear();

    EXPECT_TRUE(id.runtime_id.empty());
    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, false);
    EXPECT_FALSE(client_handler->get_client()
                     ->get_service_identifier()
                     .runtime_id.empty());
}

TEST_F(ClientHandlerTest, AsmFeatureProductIsAddeWhenDynamicEnablement)
{
    auto dynamic_enablement = true;
    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, dynamic_enablement);

    auto products_list = client_handler->get_client()->get_products();
    EXPECT_TRUE(products_list.find("ASM_FEATURES") != products_list.end());
}

TEST_F(
    ClientHandlerTest, AsmFeatureProductIsNotAddeWhenDynamicEnablementDisabled)
{
    auto dynamic_enablement = false;

    // Clear rules file so at least some other products are added
    settings.rules_file.clear();
    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, dynamic_enablement);

    auto products_list = client_handler->get_client()->get_products();
    EXPECT_TRUE(products_list.find("ASM_FEATURES") == products_list.end());
}

TEST_F(ClientHandlerTest, SomeProductsDependOnDynamicEngineBeingSet)
{
    { // When rules file is not set, products are added
        settings.rules_file.clear();
        auto client_handler = remote_config::client_handler::from_settings(
            dds::service_identifier(id), settings, service_config, rc_settings,
            engine, true);

        auto products_list = client_handler->get_client()->get_products();
        EXPECT_TRUE(products_list.find("ASM_DATA") != products_list.end());
        EXPECT_TRUE(products_list.find("ASM_DD") != products_list.end());
        EXPECT_TRUE(products_list.find("ASM") != products_list.end());
    }

    { // When rules file is set, products not are added
        settings.rules_file = "/some/file";
        auto client_handler = remote_config::client_handler::from_settings(
            dds::service_identifier(id), settings, service_config, rc_settings,
            engine, true);

        auto products_list = client_handler->get_client()->get_products();
        EXPECT_TRUE(products_list.find("ASM_DATA") == products_list.end());
        EXPECT_TRUE(products_list.find("ASM_DD") == products_list.end());
        EXPECT_TRUE(products_list.find("ASM") == products_list.end());
    }
}

TEST_F(ClientHandlerTest, IfNoProductsAreRequiredRemoteClientIsNotGenerated)
{
    settings.rules_file = "/some/file";
    auto dynamic_enablement = false;
    auto client_handler = remote_config::client_handler::from_settings(
        dds::service_identifier(id), settings, service_config, rc_settings,
        engine, dynamic_enablement);

    EXPECT_FALSE(client_handler);
}

TEST_F(ClientHandlerTest, ItDoesNotStartIfNoRcClientGiven)
{
    auto rc_client = nullptr;
    auto client_handler = remote_config::client_handler(
        rc_client, service_config, std::move(scheduler));

    EXPECT_FALSE(client_handler.start());
}

// todo make this test work
// TEST_F(ClientHandlerTest, ItStartsScheduler)
//{
//     auto rc_client =
//         std::make_unique<mock::client>(dds::service_identifier(sid));
//     auto s = mock::scheduler(1ms, 2ms);
//
//     EXPECT_CALL(s, start)
//         .Times(1);
//
//     auto client_handler = remote_config::client_handler (
//         std::move(rc_client), service_config, std::move(s));
//
//     EXPECT_TRUE(client_handler.start());
// }

TEST_F(ClientHandlerTest, WhenRcNotAvailableItKeepsDiscovering)
{
    auto rc_client =
        std::make_unique<mock::client>(dds::service_identifier(sid));
    EXPECT_CALL(*rc_client, is_remote_config_available)
        .Times(2)
        .WillRepeatedly(Return(false));

    auto client_handler = remote_config::client_handler(
        std::move(rc_client), service_config, std::move(scheduler));
    client_handler.act();
    client_handler.act();
}

TEST_F(ClientHandlerTest, WhenPollFailsItGoesBackToDiscovering)
{
    auto rc_client =
        std::make_unique<mock::client>(dds::service_identifier(sid));
    EXPECT_CALL(*rc_client, is_remote_config_available)
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*rc_client, poll).Times(1).WillOnce(Return(false));

    auto client_handler = remote_config::client_handler(
        std::move(rc_client), service_config, std::move(scheduler));

    EXPECT_TRUE(client_handler.act());  // Discover returns true
    EXPECT_FALSE(client_handler.act()); // Poll fails
    EXPECT_TRUE(client_handler.act());  // Discover again
}

TEST_F(ClientHandlerTest, ItKeepsPollingWhileNoError)
{
    auto rc_client =
        std::make_unique<mock::client>(dds::service_identifier(sid));
    EXPECT_CALL(*rc_client, is_remote_config_available)
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*rc_client, poll).Times(3).WillRepeatedly(Return(true));

    auto client_handler = remote_config::client_handler(
        std::move(rc_client), service_config, std::move(scheduler));

    EXPECT_TRUE(client_handler.act()); // Discover returns true
    EXPECT_TRUE(client_handler.act()); // Poll
    EXPECT_TRUE(client_handler.act()); // Poll
    EXPECT_TRUE(client_handler.act()); // Poll
}

} // namespace dds
