// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "../common.hpp"
#include "remote_config/service_handler.hpp"

namespace dds {

class ServiceHandlerTest : public ::testing::Test {
public:
    service_identifier sid{
        "service", "env", "tracer_version", "app_version", "runtime_id"};
    dds::engine_settings settings;
    remote_config::settings rc_settings;
    std::shared_ptr<dds::service_config> service_config;
    std::shared_ptr<engine> engine;

    void SetUp()
    {
        service_config = std::make_shared<dds::service_config>();
        service_config->sid = sid;
        engine = engine::create();
        rc_settings.enabled = true;
    }
};

TEST_F(ServiceHandlerTest, IfRemoteConfigDisabledItDoesNotGenerateHandler)
{
    rc_settings.enabled = false;

    auto service_handler = remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, false);

    EXPECT_FALSE(service_handler);
}

TEST_F(ServiceHandlerTest, IfNoServiceConfigProvidedItDoesNotGenerateHandler)
{
    std::shared_ptr<dds::service_config> null_service_config = {};
    auto service_handler = remote_config::service_handler::from_settings(
        settings, null_service_config, rc_settings, engine, false);

    EXPECT_FALSE(service_handler);
}

TEST_F(ServiceHandlerTest, RuntimeIdIsNotGeneratedIfProvided)
{
    const char *runtime_id = "some runtime id";
    service_config->sid.runtime_id = runtime_id;

    auto service_handler = remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, false);

    EXPECT_STREQ(runtime_id, service_config->sid.runtime_id.c_str());
}

TEST_F(ServiceHandlerTest, RuntimeIdIsGeneratedWhenNotProvided)
{
    service_config->sid.runtime_id.clear();

    EXPECT_TRUE(service_config->sid.runtime_id.empty());
    remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, false);
    EXPECT_FALSE(service_config->sid.runtime_id.empty());
}

TEST_F(ServiceHandlerTest, AsmFeatureProductIsAddeWhenDynamicEnablement)
{
    auto dynamic_enablement = true;
    auto service_handler = remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, dynamic_enablement);

    auto products_list = service_handler->get_client()->get_products();
    EXPECT_TRUE(products_list.find("ASM_FEATURES") != products_list.end());
}

TEST_F(
    ServiceHandlerTest, AsmFeatureProductIsNotAddeWhenDynamicEnablementDisabled)
{
    auto dynamic_enablement = false;

    // Clear rules file so at least some other products are added
    settings.rules_file.clear();
    auto service_handler = remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, dynamic_enablement);

    auto products_list = service_handler->get_client()->get_products();
    EXPECT_TRUE(products_list.find("ASM_FEATURES") == products_list.end());
}

TEST_F(ServiceHandlerTest, SomeProductsDependOnDynamicEngineBeingSet)
{
    { // When rules file is not set, products are added
        settings.rules_file.clear();
        auto service_handler = remote_config::service_handler::from_settings(
            settings, service_config, rc_settings, engine, true);

        auto products_list = service_handler->get_client()->get_products();
        EXPECT_TRUE(products_list.find("ASM_DATA") != products_list.end());
        EXPECT_TRUE(products_list.find("ASM_DD") != products_list.end());
        EXPECT_TRUE(products_list.find("ASM") != products_list.end());
    }

    { // When rules file is set, products not are added
        settings.rules_file = "/some/file";
        auto service_handler = remote_config::service_handler::from_settings(
            settings, service_config, rc_settings, engine, true);

        auto products_list = service_handler->get_client()->get_products();
        EXPECT_TRUE(products_list.find("ASM_DATA") == products_list.end());
        EXPECT_TRUE(products_list.find("ASM_DD") == products_list.end());
        EXPECT_TRUE(products_list.find("ASM") == products_list.end());
    }
}

TEST_F(ServiceHandlerTest, IfNoProductsAreRequiredRemoteClientIsNotGenerated)
{
    settings.rules_file = "/some/file";
    auto dynamic_enablement = false;
    auto service_handler = remote_config::service_handler::from_settings(
        settings, service_config, rc_settings, engine, dynamic_enablement);

    EXPECT_FALSE(service_handler);
}
} // namespace dds
