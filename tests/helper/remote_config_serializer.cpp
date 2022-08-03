// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "remote_config/protocol/client.hpp"
#include "remote_config/protocol/client_state.hpp"
#include "remote_config/protocol/client_tracer.hpp"
#include "remote_config/protocol/config_state.hpp"
#include "remote_config/protocol/tuf/client_get_configs_request.hpp"
#include "remote_config/protocol/tuf/serializer.hpp"

namespace dds {

bool array_contains_string(const rapidjson::Value &array, const char *searched)
{
    if (!array.IsArray()) {
        return false;
    }

    bool result = false;
    for (rapidjson::Value::ConstValueIterator itr = array.Begin();
         itr != array.End(); ++itr) {
        if (itr->IsString() && strcmp(searched, itr->GetString()) == 0) {
            result = true;
        }
    }

    return result;
}

rapidjson::Value::ConstMemberIterator assert_it_contains_string(
    const rapidjson::Value &parent_field, const char *key, const char *value)
{
    rapidjson::Value::ConstMemberIterator tmp_itr =
        parent_field.FindMember(key);
    bool found = false;
    if (tmp_itr != parent_field.MemberEnd()) {
        found = true;
    }
    EXPECT_TRUE(found) << "Key " << key << " not found";
    EXPECT_EQ(rapidjson::kStringType, tmp_itr->value.GetType());
    EXPECT_EQ(value, tmp_itr->value);

    return tmp_itr;
}

rapidjson::Value::ConstMemberIterator assert_it_contains_int(
    const rapidjson::Value &parent_field, const char *key, int value)
{
    rapidjson::Value::ConstMemberIterator tmp_itr =
        parent_field.FindMember(key);
    bool found = false;
    if (tmp_itr != parent_field.MemberEnd()) {
        found = true;
    }
    EXPECT_TRUE(found) << "Key " << key << " not found";
    EXPECT_EQ(rapidjson::kNumberType, tmp_itr->value.GetType());
    EXPECT_EQ(value, tmp_itr->value);

    return tmp_itr;
}

rapidjson::Value::ConstMemberIterator assert_it_contains_bool(
    const rapidjson::Value &parent_field, const char *key, bool value)
{
    rapidjson::Value::ConstMemberIterator tmp_itr =
        parent_field.FindMember(key);
    bool found = false;
    if (tmp_itr != parent_field.MemberEnd()) {
        found = true;
    }
    EXPECT_TRUE(found) << "Key " << key << " not found";
    rapidjson::Type type = rapidjson::kTrueType;
    if (!value) {
        type = rapidjson::kFalseType;
    }
    EXPECT_EQ(type, tmp_itr->value.GetType());
    EXPECT_EQ(value, tmp_itr->value);

    return tmp_itr;
}

rapidjson::Value::ConstMemberIterator find_and_assert_type(
    const rapidjson::Value &parent_field, const char *key, rapidjson::Type type)
{
    rapidjson::Value::ConstMemberIterator tmp_itr =
        parent_field.FindMember(key);
    bool found = false;
    if (tmp_itr != parent_field.MemberEnd()) {
        found = true;
    }
    EXPECT_TRUE(found) << "Key " << key << " not found";
    EXPECT_EQ(type, tmp_itr->value.GetType())
        << "Key " << key << " not matching expected type";

    return tmp_itr;
}

TEST(RemoteConfigSerializer, RequestCanBeSerialized)
{
    std::list<remote_config::protocol::product> products;
    products.push_back(remote_config::protocol::product::ASM_DD);

    remote_config::protocol::client_tracer client_tracer("some runtime id",
        "some tracer version", "some service", "some env", "some app version");

    std::list<remote_config::protocol::config_state> config_states;
    int config_state_version = 456;
    remote_config::protocol::config_state config_state("some config_state id",
        config_state_version, "some config_state product");
    config_states.push_back(config_state);

    int targets_version = 123;
    remote_config::protocol::client_state client_state(
        targets_version, config_states, false, "", "some backend client state");

    std::string client_id("some_id");
    remote_config::protocol::client client(
        client_id, products, client_tracer, client_state);
    remote_config::protocol::tuf::client_get_configs_request request(client);

    std::string serialised_string;
    auto result =
        remote_config::protocol::tuf::serialize(request, serialised_string);

    EXPECT_EQ(remote_config::protocol::tuf::SUCCESS, result);

    // Lets transform the resulting string back to json so we can assert more
    // easily
    rapidjson::Document serialized_doc;
    serialized_doc.Parse(serialised_string);

    // Client fields
    rapidjson::Value::ConstMemberIterator client_itr =
        find_and_assert_type(serialized_doc, "client", rapidjson::kObjectType);

    assert_it_contains_string(client_itr->value, "id", "some_id");

    // Client products fields
    rapidjson::Value::ConstMemberIterator products_itr = find_and_assert_type(
        client_itr->value, "products", rapidjson::kArrayType);
    array_contains_string(products_itr->value, "ASM_DD");

    // Client tracer fields
    rapidjson::Value::ConstMemberIterator client_tracer_itr =
        find_and_assert_type(
            client_itr->value, "client_tracer", rapidjson::kObjectType);
    assert_it_contains_string(client_tracer_itr->value, "language", "php");
    assert_it_contains_string(
        client_tracer_itr->value, "runtime_id", "some runtime id");
    assert_it_contains_string(
        client_tracer_itr->value, "tracer_version", "some tracer version");
    assert_it_contains_string(
        client_tracer_itr->value, "service", "some service");
    assert_it_contains_string(client_tracer_itr->value, "env", "some env");
    assert_it_contains_string(
        client_tracer_itr->value, "app_version", "some app version");

    // Client state fields
    rapidjson::Value::ConstMemberIterator client_state_itr =
        find_and_assert_type(
            client_itr->value, "state", rapidjson::kObjectType);
    assert_it_contains_int(
        client_state_itr->value, "targets_version", targets_version);
    assert_it_contains_int(client_state_itr->value, "root_version", 1);
    assert_it_contains_bool(client_state_itr->value, "has_error", false);
    assert_it_contains_string(client_state_itr->value, "error", "");
    assert_it_contains_string(client_state_itr->value, "backend_client_state",
        "some backend client state");

    // Config state fields
    rapidjson::Value::ConstMemberIterator config_states_itr =
        find_and_assert_type(
            client_state_itr->value, "config_states", rapidjson::kArrayType);
    ;

    rapidjson::Value::ConstValueIterator itr = config_states_itr->value.Begin();
    assert_it_contains_string(*itr, "id", "some config_state id");
    assert_it_contains_int(*itr, "version", config_state_version);
    assert_it_contains_string(*itr, "product", "some config_state product");
}

} // namespace dds
