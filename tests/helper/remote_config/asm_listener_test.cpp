// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "../common.hpp"
#include "base64.h"
#include "engine.hpp"
#include "json_helper.hpp"
#include "mocks.hpp"
#include "remote_config/asm_listener.hpp"
#include "remote_config/exception.hpp"
#include "subscriber/waf.hpp"
#include <rapidjson/document.h>

namespace dds::remote_config {
namespace {

using mock::generate_config;

const std::string waf_rule =
    R"({"version":"2.1","rules":[{"id":"1","name":"rule1","tags":{"type":"flow1","category":"category1"},"conditions":[{"operator":"match_regex","parameters":{"inputs":[{"address":"arg1","key_path":[]}],"regex":".*"}}]}]})";

ACTION_P(SaveDocument, param)
{
    rapidjson::Document &document =
        *reinterpret_cast<rapidjson::Document *>(param);

    arg0.copy(document);
}

TEST(RemoteConfigAsmListener, RulesOverrideEmptyCommit)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    listener.init();
    listener.commit();

    const auto &overrides = doc["rules_override"];
    EXPECT_TRUE(overrides.IsArray());
    EXPECT_EQ(overrides.Size(), 0);

    const auto &exclusions = doc["exclusions"];
    EXPECT_TRUE(exclusions.IsArray());
    EXPECT_EQ(exclusions.Size(), 0);

    const auto &actions = doc["actions"];
    EXPECT_TRUE(actions.IsArray());
    EXPECT_EQ(actions.Size(), 0);

    const auto &custom_rules = doc["custom_rules"];
    EXPECT_TRUE(custom_rules.IsArray());
    EXPECT_EQ(custom_rules.Size(), 0);
}

TEST(RemoteConfigAsmListener, RulesOverrideSingleConfig)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    const std::string rule_override =
        R"({"rules_override": [{"rules_target": [{"tags": {"confidence": "1"}}], "on_match": ["block"]}]})";

    listener.init();
    listener.on_update(generate_config(rule_override));
    listener.commit();

    const auto &overrides = doc["rules_override"];
    EXPECT_TRUE(overrides.IsArray());
    EXPECT_EQ(overrides.Size(), 1);

    const auto &ovrd = overrides[0];
    EXPECT_TRUE(ovrd.IsObject());

    {
        auto it = ovrd.FindMember("rules_target");
        EXPECT_NE(it, ovrd.MemberEnd());
        EXPECT_TRUE(it->value.IsArray());
    }

    {
        auto it = ovrd.FindMember("on_match");
        EXPECT_NE(it, ovrd.MemberEnd());
        EXPECT_TRUE(it->value.IsArray());
    }

    const auto &exclusions = doc["exclusions"];
    EXPECT_TRUE(exclusions.IsArray());
    EXPECT_EQ(exclusions.Size(), 0);

    const auto &actions = doc["actions"];
    EXPECT_TRUE(actions.IsArray());
    EXPECT_EQ(actions.Size(), 0);

    const auto &custom_rules = doc["custom_rules"];
    EXPECT_TRUE(custom_rules.IsArray());
    EXPECT_EQ(custom_rules.Size(), 0);
}

TEST(RemoteConfigAsmListener, RulesOverrideMultipleConfigs)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    const std::string rule_override =
        R"({"rules_override": [{"rules_target": [{"tags": {"confidence": "1"}}], "on_match": ["block"]}]})";

    listener.init();
    listener.on_update(generate_config(rule_override));
    listener.on_update(generate_config(rule_override));
    listener.on_update(generate_config(rule_override));
    listener.on_update(generate_config(rule_override));
    listener.commit();

    const auto &overrides = doc["rules_override"];
    EXPECT_TRUE(overrides.IsArray());
    EXPECT_EQ(overrides.Size(), 4);

    const auto &exclusions = doc["exclusions"];
    EXPECT_TRUE(exclusions.IsArray());
    EXPECT_EQ(exclusions.Size(), 0);

    const auto &actions = doc["actions"];
    EXPECT_TRUE(actions.IsArray());
    EXPECT_EQ(actions.Size(), 0);

    const auto &custom_rules = doc["custom_rules"];
    EXPECT_TRUE(custom_rules.IsArray());
    EXPECT_EQ(custom_rules.Size(), 0);

    for (auto *it = overrides.Begin(); it != overrides.End(); ++it) {
        const auto &ovrd = *it;
        EXPECT_TRUE(ovrd.IsObject());

        {
            auto it = ovrd.FindMember("rules_target");
            EXPECT_NE(it, ovrd.MemberEnd());
            EXPECT_TRUE(it->value.IsArray());
        }

        {
            auto it = ovrd.FindMember("on_match");
            EXPECT_NE(it, ovrd.MemberEnd());
            EXPECT_TRUE(it->value.IsArray());
        }
    }
}

TEST(RemoteConfigAsmListener, RulesOverridesConfigCycling)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(2)
        .WillRepeatedly(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    const std::string rule_override =
        R"({"rules_override": [{"rules_target": [{"tags": {"confidence": "1"}}], "on_match": ["block"]}]})";
    {

        listener.init();
        listener.on_update(generate_config(rule_override));
        listener.commit();

        const auto &overrides = doc["rules_override"];
        EXPECT_TRUE(overrides.IsArray());
        EXPECT_EQ(overrides.Size(), 1);

        const auto &exclusions = doc["exclusions"];
        EXPECT_TRUE(exclusions.IsArray());
        EXPECT_EQ(exclusions.Size(), 0);

        const auto &actions = doc["actions"];
        EXPECT_TRUE(actions.IsArray());
        EXPECT_EQ(actions.Size(), 0);

        const auto &custom_rules = doc["custom_rules"];
        EXPECT_TRUE(custom_rules.IsArray());
        EXPECT_EQ(custom_rules.Size(), 0);

        for (auto *it = overrides.Begin(); it != overrides.End(); ++it) {
            const auto &ovrd = *it;
            EXPECT_TRUE(ovrd.IsObject());

            {
                auto it = ovrd.FindMember("rules_target");
                EXPECT_NE(it, ovrd.MemberEnd());
                EXPECT_TRUE(it->value.IsArray());
            }

            {
                auto it = ovrd.FindMember("on_match");
                EXPECT_NE(it, ovrd.MemberEnd());
                EXPECT_TRUE(it->value.IsArray());
            }
        }
    }

    {
        listener.init();
        listener.on_update(generate_config(rule_override));
        listener.on_update(generate_config(rule_override));
        listener.on_update(generate_config(rule_override));
        listener.commit();

        const auto &overrides = doc["rules_override"];
        EXPECT_TRUE(overrides.IsArray());
        EXPECT_EQ(overrides.Size(), 3);

        const auto &exclusions = doc["exclusions"];
        EXPECT_TRUE(exclusions.IsArray());
        EXPECT_EQ(exclusions.Size(), 0);

        const auto &actions = doc["actions"];
        EXPECT_TRUE(actions.IsArray());
        EXPECT_EQ(actions.Size(), 0);

        const auto &custom_rules = doc["custom_rules"];
        EXPECT_TRUE(custom_rules.IsArray());
        EXPECT_EQ(custom_rules.Size(), 0);

        for (auto *it = overrides.Begin(); it != overrides.End(); ++it) {
            const auto &ovrd = *it;
            EXPECT_TRUE(ovrd.IsObject());

            {
                auto it = ovrd.FindMember("rules_target");
                EXPECT_NE(it, ovrd.MemberEnd());
                EXPECT_TRUE(it->value.IsArray());
            }

            {
                auto it = ovrd.FindMember("on_match");
                EXPECT_NE(it, ovrd.MemberEnd());
                EXPECT_TRUE(it->value.IsArray());
            }
        }
    }
}

TEST(RemoteConfigAsmListener, RulesOverridesEmptyConfigThrows)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(1)
        .WillRepeatedly(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    listener.init();
    EXPECT_THROW(listener.on_update(generate_config({})),
        remote_config::error_applying_config);

    listener.commit();

    const auto &overrides = doc["rules_override"];
    EXPECT_TRUE(overrides.IsArray());
    EXPECT_EQ(overrides.Size(), 0);

    const auto &exclusions = doc["exclusions"];
    EXPECT_TRUE(exclusions.IsArray());
    EXPECT_EQ(exclusions.Size(), 0);

    const auto &actions = doc["actions"];
    EXPECT_TRUE(actions.IsArray());
    EXPECT_EQ(actions.Size(), 0);

    const auto &custom_rules = doc["custom_rules"];
    EXPECT_TRUE(custom_rules.IsArray());
    EXPECT_EQ(custom_rules.Size(), 0);
}

TEST(RemoteConfigAsmListener, RulesOverridesIncorrectTypeThrows)
{
    auto engine = mock::engine::create();

    rapidjson::Document doc;

    EXPECT_CALL(*engine, update(_, _, _))
        .Times(1)
        .WillRepeatedly(DoAll(SaveDocument(&doc)));

    remote_config::asm_listener listener(engine);

    const std::string rule_override =
        R"({"rules_override": {"rules_target": [{"tags": {"confidence": "1"}}], "on_match": ["block"]}})";

    listener.init();
    EXPECT_THROW(listener.on_update(generate_config(rule_override)),
        remote_config::error_applying_config);

    listener.commit();

    const auto &overrides = doc["rules_override"];
    EXPECT_TRUE(overrides.IsArray());
    EXPECT_EQ(overrides.Size(), 0);

    const auto &exclusions = doc["exclusions"];
    EXPECT_TRUE(exclusions.IsArray());
    EXPECT_EQ(exclusions.Size(), 0);

    const auto &actions = doc["actions"];
    EXPECT_TRUE(actions.IsArray());
    EXPECT_EQ(actions.Size(), 0);

    const auto &custom_rules = doc["custom_rules"];
    EXPECT_TRUE(custom_rules.IsArray());
    EXPECT_EQ(custom_rules.Size(), 0);
}

TEST(RemoteConfigAsmListener, RulesOverridesRealEngine)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::asm_listener listener(engine);

    listener.init();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    const std::string rule_override =
        R"({"rules_override": [{"rules_target": [{"rule_id": "1"}], "enabled":"false"}]})";
    listener.on_update(generate_config(rule_override));

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    listener.commit();
    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }
}
} // namespace
} // namespace dds::remote_config
