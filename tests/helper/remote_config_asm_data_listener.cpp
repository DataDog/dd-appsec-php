// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "base64.h"
#include "common.hpp"
#include "json_helper.hpp"
#include "remote_config/asm_data_listener.hpp"
#include "remote_config/product.hpp"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

namespace dds {

struct test_rule_data_data {
    int expiration;
    std::string value;
};

struct test_rule_data {
    std::string id;
    std::string type;
    std::vector<test_rule_data_data> data;
};

remote_config::config get_asm_data(
    const std::string &content, bool encode = true)
{
    std::string encoded_content = content;
    if (encode) {
        encoded_content = base64_encode(content);
    }

    return {"some product", "some id", encoded_content, "some path", {}, 123,
        321,
        remote_config::protocol::config_state::applied_state::UNACKNOWLEDGED,
        ""};
}

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
            data_json_entry.AddMember(
                "expiration", data_entry.expiration, alloc);
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

    return get_asm_data(buffer.get_string_ref());
}

TEST(RemoteConfigAsmDataListener, ByDefaultRulesDataIsEmpty)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
}

TEST(RemoteConfigAsmDataListener, ItParsesRulesData)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {
        {"id01", "type01", {{11, "1.2.3.4"}, {22, "5.6.7.8"}}},
        {"id02", "type02", {{33, "user1"}}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(2, rules_data_parsed.size());
    // First
    auto first_parsed = rules_data[0];
    auto first_id = first_parsed.id;
    EXPECT_STREQ("id01", rules_data_parsed[first_id].id.c_str());
    EXPECT_STREQ("type01", rules_data_parsed[first_id].type.c_str());
    // First data on first
    EXPECT_STREQ("1.2.3.4", rules_data_parsed[first_id]
                                .data[first_parsed.data[0].value]
                                .value.c_str());
    EXPECT_EQ(11, rules_data_parsed[first_id]
                      .data[first_parsed.data[0].value]
                      .expiration);
    // Second data on first
    EXPECT_STREQ("5.6.7.8", rules_data_parsed[first_id]
                                .data[first_parsed.data[1].value]
                                .value.c_str());
    EXPECT_EQ(22, rules_data_parsed[first_id]
                      .data[first_parsed.data[1].value]
                      .expiration);
    // Second
    auto second_parsed = rules_data[1];
    auto second_id = second_parsed.id;
    EXPECT_STREQ("id02", rules_data_parsed[second_id].id.c_str());
    EXPECT_STREQ("type02", rules_data_parsed[second_id].type.c_str());
    // First data on second
    EXPECT_STREQ("user1", rules_data_parsed[second_id]
                              .data[second_parsed.data[0].value]
                              .value.c_str());
    EXPECT_EQ(33, rules_data_parsed[second_id]
                      .data[second_parsed.data[0].value]
                      .expiration);
}

TEST(RemoteConfigAsmDataListener, ItMergesValuesWhenIdIsTheSame)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {
        {"id01", "type01", {{11, "1.2.3.4"}}},
        {"id01", "type01", {{22, "5.6.7.8"}}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(1, rules_data_parsed.size());
    // First
    auto first_id = rules_data[0].id;
    auto first_parsed = rules_data_parsed[first_id];
    EXPECT_STREQ("id01", first_parsed.id.c_str());
    EXPECT_STREQ("type01", first_parsed.type.c_str());
    EXPECT_EQ(2, first_parsed.data.size());

    // First data on first
    EXPECT_STREQ(
        "1.2.3.4", first_parsed.data.find("1.2.3.4")->second.value.c_str());
    EXPECT_EQ(11, first_parsed.data.find("1.2.3.4")->second.expiration);
    // Second data on first
    EXPECT_STREQ(
        "5.6.7.8", first_parsed.data.find("5.6.7.8")->second.value.c_str());
    EXPECT_EQ(22, first_parsed.data.find("5.6.7.8")->second.expiration);
}

TEST(RemoteConfigAsmDataListener, WhenIdMatchesTypeIsSecondTypeIsIgnored)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {
        {"id01", "type01", {{11, "1.2.3.4"}}},
        {"id01", "another type", {{22, "5.6.7.8"}}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(1, rules_data_parsed.size());
    // First
    auto first_id = rules_data[0].id;
    auto first_parsed = rules_data_parsed[first_id];
    EXPECT_STREQ("id01", first_parsed.id.c_str());
    EXPECT_STREQ("type01", first_parsed.type.c_str());
    EXPECT_EQ(2, first_parsed.data.size());
}

TEST(RemoteConfigAsmDataListener,
    IfTwoEntriesWithTheSameIdHaveTheSameValueItGetsLatestExpirationOnDifferentSets)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {
        {"id01", "type01", {{11, "1.2.3.4"}}},
        {"id01", "type01", {{33, "1.2.3.4"}}},
        {"id01", "type01", {{22, "1.2.3.4"}}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(1, rules_data_parsed.size());
    // First
    auto first_id = rules_data[0].id;
    auto first_parsed = rules_data_parsed[first_id];
    EXPECT_STREQ("id01", first_parsed.id.c_str());
    EXPECT_STREQ("type01", first_parsed.type.c_str());
    EXPECT_EQ(1, first_parsed.data.size());

    EXPECT_STREQ(
        "1.2.3.4", first_parsed.data.find("1.2.3.4")->second.value.c_str());
    EXPECT_EQ(33, first_parsed.data.find("1.2.3.4")->second.expiration);
}

TEST(RemoteConfigAsmDataListener,
    IfTwoEntriesWithTheSameIdHaveTheSameValueItGetsLatestExpirationInSameSet)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {{"id01", "type01",
        {{11, "1.2.3.4"}, {33, "1.2.3.4"}, {22, "1.2.3.4"}}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(1, rules_data_parsed.size());
    // First
    auto first_id = rules_data[0].id;
    auto first_parsed = rules_data_parsed[first_id];
    EXPECT_STREQ("id01", first_parsed.id.c_str());
    EXPECT_STREQ("type01", first_parsed.type.c_str());
    EXPECT_EQ(1, first_parsed.data.size());

    EXPECT_STREQ(
        "1.2.3.4", first_parsed.data.find("1.2.3.4")->second.value.c_str());
    EXPECT_EQ(33, first_parsed.data.find("1.2.3.4")->second.expiration);
}

TEST(RemoteConfigAsmDataListener, IfDataIsEmptyItDoesNotAddAnyRule)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);

    std::vector<test_rule_data> rules_data = {{"id01", "type01", {}}};

    listener.on_update(get_rules_data(rules_data));

    auto rules_data_parsed = remote_config_service->get_rules_data();

    EXPECT_EQ(0, rules_data_parsed.size());
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfContentNotInBase64)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "&&&";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config base64 encoded contents:";
    remote_config::config non_base_64_content_config =
        get_asm_data(invalid_content, false);

    try {
        listener.on_update(non_base_64_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfContentNotValidJsonContent)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "InvalidJsonContent";
    std::string error_message = "";
    std::string expected_error_message = "Invalid config json contents";
    remote_config::config invalid_json_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_json_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfNoRulesDataKey)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "{\"another_key\": 1234}";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data key missing or "
        "invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfRulesDataNotArray)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "{ \"rules_data\": 1234}";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data key missing or "
        "invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfRulesDataEntryNotObject)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "{\"rules_data\": [\"invalid\"] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data entry invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfNoId)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"data\": [{\"expiration\": 11, \"value\": "
        "\"1.2.3.4\"} ], \"type\": \"some_type\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfIdNotString)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"data\": [{\"expiration\": 11, \"value\": "
        "\"1.2.3.4\"} ], \"id\": 1234, \"type\": \"some_type\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfNoType)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"data\": [{\"expiration\": 11, \"value\": "
        "\"1.2.3.4\"} ], \"id\": \"some_id\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfTypeNotString)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"data\": [{\"expiration\": 11, \"value\": "
        "\"1.2.3.4\"} ], \"type\": 1234, \"id\": \"some_id\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfNoData)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"id\": \"some_id\", \"type\": \"some_type\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfDataNotArray)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content = "{\"rules_data\": [{\"data\": 1234, \"id\": "
                                  "\"some_id\", \"type\": \"some_type\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: rules_data missing a field or "
        "field is invalid";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}

TEST(RemoteConfigAsmDataListener, ItThrowsAnErrorIfDataEntryNotObject)
{
    auto remote_config_service = std::make_shared<service_config>();
    remote_config::asm_data_listener listener(remote_config_service);
    std::string invalid_content =
        "{\"rules_data\": [{\"data\": [ \"invalid\" ], \"id\": \"some_id\", "
        "\"type\": \"some_type\"} ] }";
    std::string error_message = "";
    std::string expected_error_message =
        "Invalid config json contents: Entry on data not a valid object";
    remote_config::config invalid_content_config =
        get_asm_data(invalid_content, true);

    try {
        listener.on_update(invalid_content_config);
    } catch (remote_config::error_applying_config &error) {
        error_message = error.what();
    }

    EXPECT_TRUE(remote_config_service->get_rules_data().empty());
    EXPECT_EQ(0, error_message.compare(0, expected_error_message.length(),
                     expected_error_message));
}
} // namespace dds