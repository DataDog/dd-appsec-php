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

bool array_contains_string(rapidjson::Value &array, const char *searched)
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

    EXPECT_TRUE(serialized_doc.HasMember("client"));

    // Client id field
    EXPECT_TRUE(serialized_doc["client"].HasMember("id"));
    EXPECT_TRUE(serialized_doc["client"]["id"].IsString());
    EXPECT_EQ("some_id", serialized_doc["client"]["id"]);

    // Client products field
    EXPECT_TRUE(serialized_doc["client"].HasMember("products"));
    EXPECT_TRUE(serialized_doc["client"]["products"].IsArray());
    EXPECT_TRUE(
        array_contains_string(serialized_doc["client"]["products"], "ASM_DD"));

    // Client tracer version
    EXPECT_TRUE(serialized_doc["client"].HasMember("client_tracer"));
    EXPECT_TRUE(serialized_doc["client"]["client_tracer"].IsObject());
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"].HasMember("language"));
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"].HasMember("runtime_id"));
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"].HasMember("tracer_version"));
    EXPECT_TRUE(serialized_doc["client"]["client_tracer"].HasMember("service"));
    EXPECT_TRUE(serialized_doc["client"]["client_tracer"].HasMember("env"));
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"].HasMember("app_version"));
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"]["language"].IsString());
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"]["runtime_id"].IsString());
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"]["tracer_version"].IsString());
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"]["service"].IsString());
    EXPECT_TRUE(serialized_doc["client"]["client_tracer"]["env"].IsString());
    EXPECT_TRUE(
        serialized_doc["client"]["client_tracer"]["app_version"].IsString());
    EXPECT_EQ("php", serialized_doc["client"]["client_tracer"]["language"]);
    EXPECT_EQ("some runtime id",
        serialized_doc["client"]["client_tracer"]["runtime_id"]);
    EXPECT_EQ("some tracer version",
        serialized_doc["client"]["client_tracer"]["tracer_version"]);
    EXPECT_EQ(
        "some service", serialized_doc["client"]["client_tracer"]["service"]);
    EXPECT_EQ("some env", serialized_doc["client"]["client_tracer"]["env"]);
    EXPECT_EQ("some app version",
        serialized_doc["client"]["client_tracer"]["app_version"]);
}

} // namespace dds
