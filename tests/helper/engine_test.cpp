// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include <engine.hpp>
#include <rapidjson/document.h>
#include <subscriber/waf.hpp>

const std::string waf_rule =
    R"({"version":"2.1","rules":[{"id":"1","name":"rule1","tags":{"type":"flow1","category":"category1"},"conditions":[{"operator":"match_regex","parameters":{"inputs":[{"address":"arg1","key_path":[]}],"regex":"^string.*"}},{"operator":"match_regex","parameters":{"inputs":[{"address":"arg2","key_path":[]}],"regex":".*"}}],"action":"record"}]})";

namespace dds {

namespace mock {
class listener : public dds::subscriber::listener {
public:
    typedef std::shared_ptr<dds::mock::listener> ptr;

    MOCK_METHOD1(
        call, std::optional<dds::subscriber::event>(dds::parameter_view &));
    MOCK_METHOD2(
        get_meta_and_metrics, void(std::map<std::string_view, std::string> &,
                                  std::map<std::string_view, double> &));
};

class subscriber : public dds::subscriber {
public:
    typedef std::shared_ptr<dds::mock::subscriber> ptr;

    MOCK_METHOD0(get_listener, dds::subscriber::listener::ptr());
    MOCK_METHOD0(get_subscriptions, std::vector<std::string_view>());
};
} // namespace mock

TEST(EngineTest, NoSubscriptors)
{
    auto e{engine::create()};
    auto ctx = e->get_context();

    parameter p = parameter::map();
    p.add("a", parameter::string("value"sv));
    auto res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);
}

TEST(EngineTest, SingleSubscriptor)
{
    auto e{engine::create()};

    mock::listener::ptr listener = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*listener, call(_))
        .WillRepeatedly(Return(subscriber::event{{}, {"block"}}));

    mock::subscriber::ptr sub = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub, get_subscriptions())
        .WillRepeatedly(Return(std::vector<std::string_view>{"a", "b"}));
    EXPECT_CALL(*sub, get_listener()).WillRepeatedly(Return(listener));

    e->subscribe(sub);

    auto ctx = e->get_context();

    parameter p = parameter::map();
    p.add("a", parameter::string("value"sv));
    auto res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("b", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("c", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);
}

TEST(EngineTest, MultipleSubscriptors)
{
    auto e{engine::create()};
    mock::listener::ptr blocker = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*blocker, call(_))
        .WillRepeatedly(Return(subscriber::event{{"some event"}, {"block"}}));
    mock::listener::ptr recorder = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*recorder, call(_))
        .WillRepeatedly(Return(subscriber::event{{"some event"}, {}}));
    mock::listener::ptr ignorer = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*ignorer, call(_)).WillRepeatedly(Return(std::nullopt));

    mock::subscriber::ptr sub1 = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub1, get_subscriptions())
        .WillRepeatedly(
            Return(std::vector<std::string_view>{"a", "b", "e", "f"}));
    EXPECT_CALL(*sub1, get_listener()).WillRepeatedly(Return(blocker));

    mock::subscriber::ptr sub2 = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub2, get_subscriptions())
        .WillRepeatedly(
            Return(std::vector<std::string_view>{"c", "d", "e", "g"}));
    EXPECT_CALL(*sub2, get_listener()).WillRepeatedly(Return(recorder));

    mock::subscriber::ptr sub3 = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub3, get_subscriptions())
        .WillRepeatedly(Return(std::vector<std::string_view>{"f", "g", "h"}));
    EXPECT_CALL(*sub3, get_listener()).WillRepeatedly(Return(ignorer));

    e->subscribe(sub1);
    e->subscribe(sub2);
    e->subscribe(sub3);

    auto ctx = e->get_context();

    parameter p = parameter::map();
    p.add("a", parameter::string("value"sv));
    auto res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("b", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("c", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::record);

    p = parameter::map();
    p.add("d", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::record);

    p = parameter::map();
    p.add("e", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("f", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("g", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::record);

    p = parameter::map();
    p.add("h", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("a", parameter::string("value"sv));
    p.add("c", parameter::string("value"sv));
    p.add("h", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    p = parameter::map();
    p.add("c", parameter::string("value"sv));
    p.add("h", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::record);
}

TEST(EngineTest, StatefulSubscriptor)
{
    auto e{engine::create()};

    mock::listener::ptr listener = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*listener, call(_))
        .Times(6)
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(subscriber::event{{}, {"block"}}))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(subscriber::event{{}, {"block"}}));

    mock::subscriber::ptr sub = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub, get_subscriptions())
        .WillRepeatedly(
            Return(std::vector<std::string_view>{"sub1", "sub2", "final"}));
    EXPECT_CALL(*sub, get_listener()).WillRepeatedly(Return(listener));

    e->subscribe(sub);

    auto ctx = e->get_context();

    parameter p = parameter::map();
    p.add("sub1", parameter::string("value"sv));
    auto res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("sub2", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("irrelevant", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("final", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);

    auto ctx2 = e->get_context();

    p = parameter::map();
    p.add("final", parameter::string("value"sv));
    res = ctx2.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("sub1", parameter::string("value"sv));
    res = ctx2.publish(std::move(p));
    EXPECT_FALSE(res);

    p = parameter::map();
    p.add("sub2", parameter::string("value"sv));
    res = ctx2.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::block);
}

TEST(EngineTest, CustomActions)
{
    auto e{engine::create(engine_settings::default_trace_rate_limit,
        {{"redirect",
            {engine::action_type::redirect, {{"url", "datadoghq.com"}}}}})};

    mock::listener::ptr listener = mock::listener::ptr(new mock::listener());
    EXPECT_CALL(*listener, call(_))
        .WillRepeatedly(Return(subscriber::event{{}, {"redirect"}}));

    mock::subscriber::ptr sub = mock::subscriber::ptr(new mock::subscriber());
    EXPECT_CALL(*sub, get_subscriptions())
        .WillRepeatedly(Return(std::vector<std::string_view>{"a", "b"}));
    EXPECT_CALL(*sub, get_listener()).WillRepeatedly(Return(listener));

    e->subscribe(sub);

    auto ctx = e->get_context();

    parameter p = parameter::map();
    p.add("a", parameter::string("value"sv));
    auto res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::redirect);

    p = parameter::map();
    p.add("b", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::redirect);

    p = parameter::map();
    p.add("c", parameter::string("value"sv));
    res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);
}

TEST(EngineTest, WafSubscriptorBasic)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    auto ctx = e->get_context();

    auto p = parameter::map();
    p.add("arg1", parameter::string("string 1"sv));
    p.add("arg2", parameter::string("string 3"sv));

    auto res = ctx.publish(std::move(p));
    EXPECT_TRUE(res);
    EXPECT_EQ(res->type, engine::action_type::record);
    EXPECT_EQ(res->events.size(), 1);
    for (auto &match : res->events) {
        rapidjson::Document doc;
        doc.Parse(match);
        EXPECT_FALSE(doc.HasParseError());
        EXPECT_TRUE(doc.IsObject());
    }
}

TEST(EngineTest, WafSubscriptorInvalidParam)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(waf_rule, meta, metrics));

    auto ctx = e->get_context();

    auto p = parameter::array();

    EXPECT_THROW(ctx.publish(std::move(p)), invalid_object);
}

TEST(EngineTest, WafSubscriptorTimeout)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    auto e{engine::create()};
    e->subscribe(waf::instance::from_string(waf_rule, meta, metrics, 0));

    auto ctx = e->get_context();

    auto p = parameter::map();
    p.add("arg1", parameter::string("string 1"sv));
    p.add("arg2", parameter::string("string 3"sv));

    auto res = ctx.publish(std::move(p));
    EXPECT_FALSE(res);
}

TEST(EngineTest, ActionsParser)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage","type": "block_request","parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 1);
    EXPECT_NE(parsed_actions.find("cabbage"), parsed_actions.end());

    auto &action_spec = parsed_actions["cabbage"];
    EXPECT_EQ(action_spec.type, engine::action_type::block);
    EXPECT_EQ(action_spec.parameters.size(), 6);
    EXPECT_STREQ(action_spec.parameters["status_code"].c_str(), "100");
    EXPECT_STREQ(action_spec.parameters["type"].c_str(), "html");
    EXPECT_STREQ(
        action_spec.parameters["double"].substr(0, 5).c_str(), "1.523");
    EXPECT_STREQ(action_spec.parameters["negative"].c_str(), "-44");
    EXPECT_STREQ(action_spec.parameters["true"].c_str(), "true");
    EXPECT_STREQ(action_spec.parameters["false"].c_str(), "false");
}

TEST(EngineTest, ActionsParserNoId)
{
    const std::string action_ruleset =
        R"({"actions": [{"type": "block_request","parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserWrongIdType)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": 25, "type": "block_request","parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserNoType)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage", "parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserWrongTypeType)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage", "type": false, "parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserNoParameters)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage", "type": "block_request"}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserWrongParametersType)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage", "type": "block_request", "parameters": []}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 0);
}

TEST(EngineTest, ActionsParserMultiple)
{
    const std::string action_ruleset =
        R"({"actions": [{"id": "cabbage","type": "block_request","parameters": {"status_code": 100,"type": "html","double": 1.523, "negative": -44, "true": true, "false": false, "invalid": []}},{"id": "tomato","type": "block_request","parameters": {}}]})";

    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(action_ruleset);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(doc.IsObject());

    auto parsed_actions = engine::parse_actions(doc, {});
    EXPECT_EQ(parsed_actions.size(), 2);
}

} // namespace dds
