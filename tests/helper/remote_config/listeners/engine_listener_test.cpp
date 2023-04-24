// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "../../common.hpp"
#include "../mocks.hpp"
#include "base64.h"
#include "json_helper.hpp"
#include "remote_config/exception.hpp"
#include "remote_config/listeners/engine_listener.hpp"
#include "remote_config/product.hpp"
#include "subscriber/waf.hpp"
#include <rapidjson/writer.h>

const std::string waf_rule =
    R"({"version":"2.1","rules":[{"id":"1","name":"rule1","tags":{"type":"flow1","category":"category1"},"conditions":[{"operator":"match_regex","parameters":{"inputs":[{"address":"arg1","key_path":[]}],"regex":".*"}}]}]})";

namespace dds::remote_config {

using mock::generate_config;

namespace {

struct test_rule_data_data {
    std::optional<uint64_t> expiration;
    std::string value;
};

struct test_rule_data {
    std::string id;
    std::string type;
    std::vector<test_rule_data_data> data;
};

remote_config::config get_rules_data(std::vector<test_rule_data> data)
{
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &alloc = document.GetAllocator();

    document.SetObject();

    rapidjson::Value rules_json(rapidjson::kArrayType);

    for (const auto &rule_data : data) {
        rapidjson::Value rule_data_entry(rapidjson::kObjectType);

        // Generate data on entry
        rapidjson::Value data_json(rapidjson::kArrayType);
        for (const auto &data_entry : rule_data.data) {
            rapidjson::Value data_json_entry(rapidjson::kObjectType);
            data_json_entry.AddMember("value", data_entry.value, alloc);
            if (data_entry.expiration) {
                data_json_entry.AddMember(
                    "expiration", data_entry.expiration.value(), alloc);
            }
            data_json.PushBack(data_json_entry, alloc);
        }

        rule_data_entry.AddMember("id", rule_data.id, alloc);
        rule_data_entry.AddMember("type", rule_data.type, alloc);
        rule_data_entry.AddMember("data", data_json, alloc);
        rules_json.PushBack(rule_data_entry, alloc);
    }

    document.AddMember("rules_data", rules_json, alloc);

    dds::string_buffer buffer;
    rapidjson::Writer<decltype(buffer)> writer(buffer);
    document.Accept(writer);

    return generate_config("ASM_DATA", buffer.get_string_ref());
}
} // namespace

TEST(RemoteConfigEngineListener, RuleUpdate)
{
    const std::string rules =
        R"({"version": "2.2", "rules": [{"id": "some id", "name": "some name", "tags":
            {"type": "lfi", "category": "attack_attempt"}, "conditions": [{"parameters":
            {"inputs": [{"address": "server.request.query"} ], "list": ["/other/url"] },
            "operator": "phrase_match"} ], "on_match": ["block"] } ] })";

    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;
    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(rules, meta, metrics));

    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("server.request.query", parameter::string("/anotherUrl"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }

    const std::string new_rules =
        R"({"version": "2.2", "rules": [{"id": "some id", "name": "some name","tags":
            {"type": "lfi", "category": "attack_attempt"}, "conditions":[{"parameters":
            {"inputs": [{"address": "server.request.query"} ], "list":
            ["/anotherUrl"] }, "operator": "phrase_match"} ], "on_match": ["block"]}]})";

    remote_config::engine_listener listener(e);
    listener.init();
    listener.on_update(generate_config("ASM_DD", new_rules));
    listener.commit();

    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("server.request.query", parameter::string("/anotherUrl"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::block);
        EXPECT_EQ(res->events.size(), 1);
    }
}

TEST(RemoteConfigEngineListener, RuleUpdateFallback)
{
    const std::string rules =
        R"({"version": "2.2", "rules": [{"id": "some id", "name": "some name", "tags":
            {"type": "lfi", "category": "attack_attempt"}, "conditions": [{"parameters":
            {"inputs": [{"address": "server.request.query"} ], "list": ["/a/url"] },
            "operator": "phrase_match"} ], "on_match": ["block"] } ] })";

    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;
    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(rules, meta, metrics));

    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("server.request.query", parameter::string("/a/url"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::block);
        EXPECT_EQ(res->events.size(), 1);
    }

    remote_config::engine_listener listener(e, create_sample_rules_ok());
    listener.init();
    listener.on_unapply(generate_config("ASM_DD", std::string("")));
    listener.commit();

    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("server.request.query", parameter::string("/a/url"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }
}

TEST(RemoteConfigEngineListener, RuleOverrideUpdateDisableRule)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::engine_listener listener(engine);
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
    listener.on_update(generate_config("ASM", rule_override));

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

TEST(RemoteConfigEngineListener, RuleOverrideUpdateSetOnMatch)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::engine_listener listener(engine);

    listener.init();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::record);
    }

    const std::string rule_override =
        R"({"rules_override": [{"rules_target": [{"tags": {"type": "flow1"}}], "on_match": ["block"]}]})";
    listener.on_update(generate_config("ASM", rule_override));

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::record);
    }

    listener.commit();
    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::block);
    }
}

TEST(RemoteConfigEngineListener, RuleOverrideAndActionsUpdate)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::engine_listener listener(engine);

    listener.init();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::record);
    }
    const std::string update =
        R"({"actions": [{"id": "redirect", "type": "redirect_request", "parameters":
            {"status_code": "303", "location": "localhost"}}],"rules_override":
            [{"rules_target": [{"rule_id": "1"}], "on_match": ["redirect"]}]})";

    listener.on_update(generate_config("ASM", update));

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::record);
    }

    listener.commit();
    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::redirect);
    }
}

TEST(RemoteConfigEngineListener, ExclusionsUpdatePasslistRule)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::engine_listener listener(engine);

    listener.init();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    const std::string update =
        R"({"exclusions":[{"id":1,"rules_target":[{"rule_id":1}]}]})";
    listener.on_update(generate_config("ASM", update));

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

TEST(RemoteConfigEngineListener, CustomRulesUpdate)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto engine{dds::engine::create()};
    engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    remote_config::engine_listener listener(engine);

    listener.init();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg3", parameter::string("custom rule"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }

    const std::string update =
        R"({"custom_rules":[{"id":"1","name":"custom_rule1","tags":{"type":"custom",
            "category":"custom"},"conditions":[{"operator":"match_regex","parameters":
            {"inputs":[{"address":"arg3","key_path":[]}],"regex":"^custom.*"}}],
            "on_match":["block"]}]})";
    listener.on_update(generate_config("ASM", update));

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg3", parameter::string("custom rule"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }

    listener.commit();
    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg3", parameter::string("custom rule"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    listener.init();
    listener.on_update(generate_config("ASM", R"({"custom_rules":[]})"));
    listener.commit();

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg1", parameter::string("value"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
    }

    {
        auto ctx = engine->get_context();

        auto p = parameter::map();
        p.add("arg3", parameter::string("custom rule"sv));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }
}

TEST(RemoteConfigEngineListener, RuleDataUpdate)
{
    std::string ip = "1.2.3.4";
    const std::string waf_rule_with_data =
        R"({"version":"2.1","rules":[{"id":"blk-001-001","name":"Block IP Addresses",
            "tags":{"type":"block_ip","category":"security_response"},"conditions":
            [{"parameters":{"inputs":[{"address":"http.client_ip"}],"data":"blocked_ips"},
            "operator":"ip_match"}],"transformers":[],"on_match":["block"]}]})";

    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;
    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(waf_rule_with_data, meta, metrics));

    std::vector<test_rule_data> rules_data = {
        {"blocked_ips", "data_with_expiration", {{std::nullopt, ip}}}};

    remote_config::engine_listener listener(e);
    listener.init();

    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("http.client_ip", parameter::string(ip));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }

    listener.on_update(get_rules_data(rules_data));
    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("http.client_ip", parameter::string(ip));

        auto res = ctx.publish(std::move(p));
        EXPECT_FALSE(res);
    }

    listener.commit();
    {
        auto ctx = e->get_context();

        auto p = parameter::map();
        p.add("http.client_ip", parameter::string(ip));

        auto res = ctx.publish(std::move(p));
        EXPECT_TRUE(res);
        EXPECT_EQ(res->type, engine::action_type::block);
        EXPECT_EQ(res->events.size(), 1);
    }
}

} // namespace dds::remote_config
