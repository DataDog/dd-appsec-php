// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "remote_config/protocol/client.hpp"
#include "remote_config/protocol/client_tracer.hpp"
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
    EXPECT_EQ(rapidjson::kStringType, tmp_itr->value.GetType());
    EXPECT_EQ(value, tmp_itr->value);

    return tmp_itr;
}

rapidjson::Value::ConstMemberIterator find_and_assert_type(
    const rapidjson::Value &parent_field, const char *key, rapidjson::Type type)
{
    rapidjson::Value::ConstMemberIterator tmp_itr =
        parent_field.FindMember(key);
    EXPECT_EQ(type, tmp_itr->value.GetType());

    return tmp_itr;
}

TEST(RemoteConfigSerializer, RequestCanBeSerialized)
{
    std::list<remote_config::protocol::product> products;
    products.push_back(remote_config::protocol::product::ASM_DD);

    remote_config::protocol::client_tracer client_tracer("some runtime id",
        "some tracer version", "some service", "some env", "some app version");

    std::string client_id("some_id");
    remote_config::protocol::client client(client_id, products, client_tracer);
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
    assert_it_contains_string(client_tracer_itr->value, "id", "some_id");

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
}

} // namespace dds
