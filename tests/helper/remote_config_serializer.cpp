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
#include "remote_config/protocol/tuf/common.hpp"
#include "remote_config/protocol/tuf/get_configs_request.hpp"
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

int config_state_version = 456;
int targets_version = 123;

remote_config::protocol::client get_client()
{
    std::vector<std::string> products = {"ASM_DD"};

    std::string runtime_id = "some runtime id";
    std::string tracer_version = "some tracer version";
    std::string service = "some service";
    std::string env = "some env";
    std::string app_version = "some app version";
    remote_config::protocol::client_tracer client_tracer(
        runtime_id, tracer_version, service, env, app_version);

    std::vector<remote_config::protocol::config_state> config_states;

    std::string config_state = "some config_state id";
    std::string config_state_product = "some config_state product";
    remote_config::protocol::config_state cs(std::move(config_state),
        config_state_version, std::move(config_state_product));
    config_states.push_back(cs);

    std::string error = "";
    std::string backend_client_state = "some backend client state";
    remote_config::protocol::client_state client_s(
        targets_version, config_states, false, error, backend_client_state);

    std::string client_id("some_id");
    remote_config::protocol::client client(
        client_id, products, client_tracer, client_s);

    return client;
}

std::vector<remote_config::protocol::cached_target_files>
get_cached_target_files()
{
    std::vector<remote_config::protocol::cached_target_files>
        cached_target_files;

    std::vector<remote_config::protocol::cached_target_files_hash> first_hashes;
    std::string first_hash_algorithm = "first hash algorithm";
    std::string first_hash_hash = "first hash hash";
    remote_config::protocol::cached_target_files_hash first_hash(
        first_hash_algorithm, first_hash_hash);
    first_hashes.push_back(first_hash);
    std::string first_path = "first some path";
    remote_config::protocol::cached_target_files first(
        first_path, 1, first_hashes);
    cached_target_files.push_back(first);

    std::vector<remote_config::protocol::cached_target_files_hash>
        second_hashes;
    std::string second_hash_algorithm = "second hash algorithm";
    std::string second_hash_hash = "second hash hash";
    remote_config::protocol::cached_target_files_hash second_hash(
        second_hash_algorithm, second_hash_hash);
    second_hashes.push_back(second_hash);
    std::string second_path = "second some path";
    remote_config::protocol::cached_target_files second(
        second_path, 1, second_hashes);
    cached_target_files.push_back(second);

    return cached_target_files;
}

TEST(RemoteConfigSerializer, RequestCanBeSerializedWithClientField)
{
    remote_config::protocol::client client = get_client();
    std::vector<remote_config::protocol::cached_target_files> vector =
        get_cached_target_files();
    remote_config::protocol::get_configs_request request(client, vector);

    std::optional<std::string> serialised_string;
    serialised_string = remote_config::protocol::serialize(request);

    EXPECT_TRUE(serialised_string);

    // Lets transform the resulting string back to json so we can assert more
    // easily
    rapidjson::Document serialized_doc;
    serialized_doc.Parse(serialised_string.value());

    // Client fields
    rapidjson::Value::ConstMemberIterator client_itr =
        find_and_assert_type(serialized_doc, "client", rapidjson::kObjectType);

    assert_it_contains_string(client_itr->value, "id", "some_id");
    assert_it_contains_bool(client_itr->value, "is_tracer", true);

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

TEST(RemoteConfigSerializer, RequestCanBeSerializedWithCachedTargetFields)
{
    remote_config::protocol::client client = get_client();
    std::vector<remote_config::protocol::cached_target_files> vector =
        get_cached_target_files();
    remote_config::protocol::get_configs_request request(client, vector);

    std::optional<std::string> serialised_string;
    serialised_string = remote_config::protocol::serialize(request);

    EXPECT_TRUE(serialised_string);

    // Lets transform the resulting string back to json so we can assert more
    // easily
    rapidjson::Document serialized_doc;
    serialized_doc.Parse(serialised_string.value());

    // cached_target_files fields
    rapidjson::Value::ConstMemberIterator cached_target_files_itr =
        find_and_assert_type(
            serialized_doc, "cached_target_files", rapidjson::kArrayType);

    EXPECT_EQ(2, cached_target_files_itr->value.Size());

    rapidjson::Value::ConstValueIterator first =
        cached_target_files_itr->value.Begin();
    assert_it_contains_string(*first, "path", "first some path");
    assert_it_contains_int(*first, "length", 1);

    // Cached target file hash of first
    rapidjson::Value::ConstMemberIterator first_cached_target_files_hash =
        find_and_assert_type(*first, "hashes", rapidjson::kArrayType);
    EXPECT_EQ(1, first_cached_target_files_hash->value.Size());
    assert_it_contains_string(*first_cached_target_files_hash->value.Begin(),
        "algorithm", "first hash algorithm");
    assert_it_contains_string(*first_cached_target_files_hash->value.Begin(),
        "hash", "first hash hash");

    rapidjson::Value::ConstValueIterator second =
        std::next(cached_target_files_itr->value.Begin());
    assert_it_contains_string(*second, "path", "second some path");
    assert_it_contains_int(*second, "length", 1);

    // Cached target file hash of second
    rapidjson::Value::ConstMemberIterator second_cached_target_files_hash =
        find_and_assert_type(*second, "hashes", rapidjson::kArrayType);
    EXPECT_EQ(1, second_cached_target_files_hash->value.Size());
    assert_it_contains_string(*second_cached_target_files_hash->value.Begin(),
        "algorithm", "second hash algorithm");
    assert_it_contains_string(*second_cached_target_files_hash->value.Begin(),
        "hash", "second hash hash");
}

} // namespace dds
