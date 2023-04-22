// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "../../common.hpp"
#include "../mocks.hpp"
#include "base64.h"
#include "remote_config/exception.hpp"
#include "remote_config/listeners/engine_listener.hpp"
#include "remote_config/product.hpp"
#include "subscriber/waf.hpp"

namespace dds::remote_config {

using mock::generate_config;

TEST(RemoteConfigEngineListener, UpdateEngine)
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
        R"({"version": "2.2", "rules": [{"id": "some id", "name": "some name",
     "tags":
     {"type": "lfi", "category": "attack_attempt"}, "conditions":
     [{"parameters":
     {"inputs": [{"address": "server.request.query"} ], "list":
     ["/anotherUrl"] }, "operator": "phrase_match"} ], "on_match": ["block"] }
     ] })";

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

/*TEST(RemoteConfigAsmDdAggregator, ItFallbackEngine)*/
/*{*/
/*const std::string rules =*/
/*R"({"version": "2.2", "rules": [{"id": "some id", "name": "some name",
 * "tags":
 * {"type": "lfi", "category": "attack_attempt"}, "conditions":
 * [{"parameters":
 * {"inputs": [{"address": "server.request.query"} ], "list": ["/a/url"] },
 * "operator": "phrase_match"} ], "on_match": ["block"] } ] })";*/

/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/
/*auto e{engine::create()};*/
/*e->subscribe(waf::instance::from_string(rules, meta, metrics));*/

/*{*/
/*auto ctx = e->get_context();*/

/*auto p = parameter::map();*/
/*p.add("server.request.query", parameter::string("/a/url"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::block);*/
/*EXPECT_EQ(res->events.size(), 1);*/
/*}*/

/*remote_config::asm_dd_aggregator aggregator(e,
 * create_sample_rules_ok());*/
/*aggregator.on_unapply(get_asm_dd_data(""));*/

/*{*/
/*auto ctx = e->get_context();*/

/*auto p = parameter::map();*/
/*p.add("server.request.query", parameter::string("/a/url"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmAggregator, EngineRulesOverrideDisableRule)*/
/*{*/
/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/

/*auto engine{dds::engine::create()};*/
/*engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));*/

/*remote_config::asm_aggregator aggregator(engine);*/

/*aggregator.init();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*const std::string rule_override =*/
/*R"({"rules_override": [{"rules_target": [{"rule_id": "1"}],
 * "enabled":"false"}]})";*/
/*aggregator.on_update(generate_config(rule_override));*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*aggregator.commit();*/
/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmAggregator, EngineRulesOverrideSetOnMatch)*/
/*{*/
/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/

/*auto engine{dds::engine::create()};*/
/*engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));*/

/*remote_config::asm_aggregator aggregator(engine);*/

/*aggregator.init();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::record);*/
/*}*/

/*const std::string rule_override =*/
/*R"({"rules_override": [{"rules_target": [{"tags": {"type": "flow1"}}],
 * "on_match": ["block"]}]})";*/
/*aggregator.on_update(generate_config(rule_override));*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::record);*/
/*}*/

/*aggregator.commit();*/
/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::block);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmAggregator, EngineRulesOverrideAndActionDefinition)*/
/*{*/
/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/

/*auto engine{dds::engine::create()};*/
/*engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));*/

/*remote_config::asm_aggregator aggregator(engine);*/

/*aggregator.init();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::record);*/
/*}*/
/*const std::string update =*/
/*R"({"actions": [{"id": "redirect", "type": "redirect_request",
 * "parameters":
 * {"status_code": "303", "location": "localhost"}}],"rules_override":
 * [{"rules_target": [{"rule_id": "1"}], "on_match": ["redirect"]}]})";*/
/*aggregator.on_update(generate_config(update));*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::record);*/
/*}*/

/*aggregator.commit();*/
/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::redirect);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmAggregator, EngineExclusionPasslistRule)*/
/*{*/
/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/

/*auto engine{dds::engine::create()};*/
/*engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));*/

/*remote_config::asm_aggregator aggregator(engine);*/

/*aggregator.init();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*const std::string update =*/
/*R"({"exclusions":[{"id":1,"rules_target":[{"rule_id":1}]}]})";*/
/*aggregator.on_update(generate_config(update));*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*aggregator.commit();*/
/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmAggregator, EngineCustomRules)*/
/*{*/
/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/

/*auto engine{dds::engine::create()};*/
/*engine->subscribe(waf::instance::from_string(waf_rule, meta, metrics));*/

/*remote_config::asm_aggregator aggregator(engine);*/

/*aggregator.init();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg3", parameter::string("custom rule"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/

/*const std::string update =*/
/*R"({"custom_rules":[{"id":"1","name":"custom_rule1","tags":{"type":"custom","category":"custom"},"conditions":[{"operator":"match_regex","parameters":{"inputs":[{"address":"arg3","key_path":[]}],"regex":"^custom.*"}}],"on_match":["block"]}]})";*/
/*aggregator.on_update(generate_config(update));*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg3", parameter::string("custom rule"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/

/*aggregator.commit();*/
/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg3", parameter::string("custom rule"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*aggregator.init();*/
/*aggregator.on_update(generate_config(R"({"custom_rules":[]})"));*/
/*aggregator.commit();*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg1", parameter::string("value"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*}*/

/*{*/
/*auto ctx = engine->get_context();*/

/*auto p = parameter::map();*/
/*p.add("arg3", parameter::string("custom rule"sv));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/
/*}*/

/*TEST(RemoteConfigAsmDataListener, ItUpdatesEngine)*/
/*{*/
/*std::string ip = "1.2.3.4";*/
/*const std::string waf_rule_with_data =*/
/*R"({"version":"2.1","rules":[{"id":"blk-001-001","name":"Block IP
 * Addresses","tags":{"type":"block_ip","category":"security_response"},"conditions":[{"parameters":{"inputs":[{"address":"http.client_ip"}],"data":"blocked_ips"},"operator":"ip_match"}],"transformers":[],"on_match":["block"]}]})";*/

/*std::map<std::string_view, std::string> meta;*/
/*std::map<std::string_view, double> metrics;*/
/*auto e{engine::create()};*/
/*e->subscribe(waf::instance::from_string(waf_rule_with_data, meta,
 * metrics));*/

/*std::vector<test_rule_data> rules_data = {*/
/*{"blocked_ips", "data_with_expiration", {{std::nullopt, ip}}}};*/

/*remote_config::asm_data_listener listener(e);*/
/*{*/
/*auto ctx = e->get_context();*/

/*auto p = parameter::map();*/
/*p.add("http.client_ip", parameter::string(ip));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/

/*listener.on_update(get_rules_data(rules_data));*/
/*{*/
/*auto ctx = e->get_context();*/

/*auto p = parameter::map();*/
/*p.add("http.client_ip", parameter::string(ip));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_FALSE(res);*/
/*}*/

/*listener.commit();*/
/*{*/
/*auto ctx = e->get_context();*/

/*auto p = parameter::map();*/
/*p.add("http.client_ip", parameter::string(ip));*/

/*auto res = ctx.publish(std::move(p));*/
/*EXPECT_TRUE(res);*/
/*EXPECT_EQ(res->type, engine::action_type::block);*/
/*EXPECT_EQ(res->events.size(), 1);*/
/*}*/
/*}*/

} // namespace dds::remote_config
