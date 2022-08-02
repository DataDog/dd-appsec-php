// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "remote_config/protocol/client.hpp"
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

    std::string client_id("some_id");
    remote_config::protocol::client client(client_id, products);
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
}

} // namespace dds
