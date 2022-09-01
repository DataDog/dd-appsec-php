// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "common.hpp"
#include "remote_config/api.hpp"
#include "remote_config/client.hpp"
#include "remote_config/protocol/client.hpp"
#include "remote_config/protocol/client_state.hpp"
#include "remote_config/protocol/client_tracer.hpp"
#include "remote_config/protocol/config_state.hpp"
#include "remote_config/protocol/tuf/common.hpp"
#include "remote_config/protocol/tuf/get_configs_request.hpp"
#include "remote_config/protocol/tuf/serializer.hpp"

namespace dds {

class cout_listener : remote_config::product_listener {
public:
    void on_update(std::vector<remote_config::config> configs) override
    {
        std::cout << std::endl
                  << "Product update " << std::endl
                  << "---------------" << std::endl;
        std::cout << "Updating " << configs[0].get_product() << " with config "
                  << configs[0].get_id() << " version "
                  << configs[0].get_version() << std::endl;
    };
    void on_unapply(std::vector<remote_config::config> configs) override{};
};

namespace mock {

// The simple custom action
ACTION_P(set_response_body, response) { arg1.assign(response); }

class api : public remote_config::http_api {
public:
    MOCK_METHOD(remote_config::protocol::remote_config_result, get_configs,
        (std::string request, std::string &response_body), (override));
};

class listener_mock : public remote_config::product_listener {
public:
    MOCK_METHOD(void, on_update, (std::vector<remote_config::config> configs),
        (override));
    MOCK_METHOD(void, on_unapply, (std::vector<remote_config::config> configs),
        (override));
};
} // namespace mock

std::string id = "some id";
std::string runtime_id = "some runtime id";
std::string tracer_version = "some tracer version";
std::string service = "some service";
std::string env = "some env";
std::string app_version = "some app version";
std::string backend_client_state = "";
int target_version = 0;
std::string features = "FEATURES";
std::string asm_dd = "ASM_DD";
std::string apm_sampling = "APM_SAMPLING";
std::vector<std::string> products = {asm_dd, features};

std::vector<remote_config::product> get_products()
{
    std::vector<remote_config::product> _products;

    std::vector<remote_config::product_listener *> listeners;
    for (std::string &p_str : products) {
        remote_config::product _p(p_str, listeners);
        _products.push_back(_p);
    }

    return _products;
}

remote_config::protocol::client generate_client(bool generate_state)
{
    remote_config::protocol::client_tracer client_tracer(
        runtime_id, tracer_version, service, env, app_version);

    std::vector<remote_config::protocol::config_state> config_states;
    int _target_version;
    std::string _backend_client_state;
    if (generate_state) {
        // All these states are extracted from the harcoded request/response
        std::string id00 = "luke.steensen";
        std::string product00(features);
        remote_config::protocol::config_state cs00(id00, 3, product00);
        std::string id01 = "2.test1.config";
        std::string product01(features);
        remote_config::protocol::config_state cs01(id01, 1, product01);

        config_states.push_back(cs00);
        config_states.push_back(cs01);
        _target_version = 27487156;
        // This field is extracted from the harcoded resposne
        _backend_client_state =
            "eyJ2ZXJzaW9uIjoxLCJzdGF0ZSI6eyJmaWxlX2hhc2hlcyI6WyJSKzJDVmtldERzYW"
            "5pWkdJa0ZaZFJNT2FYa3VzMDF1elQ1M3pnemlSTGE0PSIsIkIwWmM3T1IrUlVLcndO"
            "b0VEWjY3UXV5WElra2cxb2NHVWR3ekZsS0dDZFU9IiwieHFqTlUxTUxXU3BRbDZNak"
            "xPU2NvSUJ2b3lSelZrdzZzNGErdXVwOWgwQT0iXX19";

    } else {
        _target_version = target_version; // Default target version
        _backend_client_state = "";
    }
    std::string error = "";
    remote_config::protocol::client_state client_state(
        _target_version, config_states, false, error, _backend_client_state);

    remote_config::protocol::client client(
        id, products, client_tracer, client_state);

    return client;
}

std::string generate_empty_response()
{
    return ("{\"roots\": [], \"targets\": \"\", \"target_files\": [], "
            "\"client_configs\": [] "
            "}");
}

std::string generate_example_response(bool add_target_files)
{
    std::string target_files = "";

    if (add_target_files) {
        target_files =
            "{\"path\": "
            "\"employee/FEATURES/2.test1.config/config\", \"raw\": "
            "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
            "{\"path\": \"datadog/2/FEATURES/luke.steensen/config\", \"raw\": "
            "\"aGVsbG8gdmVjdG9yIQ==\"}";
    }
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyJzaWduYXR1cmVzIjogW3sia2V5aWQiOiAiNWM0ZWNlNDEyNDFhMWJiNTEzZjZlM2U1"
        "ZGY3NGFiN2Q1MTgzZGZmZmJkNzFiZmQ0MzEyNzkyMGQ4ODA1NjlmZCIsICJzaWciOiAiND"
        "liOTBmNWY0YmZjMjdjY2JkODBkOWM4NDU4ZDdkMjJiYTlmYTA4OTBmZDc3NWRkMTE2YzUy"
        "OGIzNmRkNjA1YjFkZjc2MWI4N2I2YzBlYjliMDI2NDA1YTEzZWZlZjQ4Mjc5MzRkNmMyNW"
        "E3ZDZiODkyNWZkYTg5MjU4MDkwMGYifSBdLCAic2lnbmVkIjogeyJfdHlwZSI6ICJ0YXJn"
        "ZXRzIiwgImN1c3RvbSI6IHsib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAiZXlKMlpYSnphVz"
        "l1SWpveExDSnpkR0YwWlNJNmV5Sm1hV3hsWDJoaGMyaGxjeUk2V3lKU0t6SkRWbXRsZEVS"
        "ellXNXBXa2RKYTBaYVpGSk5UMkZZYTNWek1ERjFlbFExTTNwbmVtbFNUR0UwUFNJc0lrSX"
        "dXbU0zVDFJclVsVkxjbmRPYjBWRVdqWTNVWFY1V0VscmEyY3hiMk5IVldSM2VrWnNTMGRE"
        "WkZVOUlpd2llSEZxVGxVeFRVeFhVM0JSYkRaTmFreFBVMk52U1VKMmIzbFNlbFpyZHpaek"
        "5HRXJkWFZ3T1dnd1FUMGlYWDE5In0sICJleHBpcmVzIjogIjIwMjItMTEtMDRUMTM6MzE6"
        "NTlaIiwgInNwZWNfdmVyc2lvbiI6ICIxLjAuMCIsICJ0YXJnZXRzIjogeyJkYXRhZG9nLz"
        "IvRkVBVFVSRVMvZHluYW1pY19yYXRlcy9jb25maWciOiB7ImN1c3RvbSI6IHsidiI6IDM2"
        "NzQwIH0sICJoYXNoZXMiOiB7InNoYTI1NiI6ICIwNzQ2NWNlY2U0N2U0NTQyYWJjMGRhMD"
        "QwZDllYmI0MmVjOTcyMjQ5MjBkNjg3MDY1MWRjMzMxNjUyODYwOWQ1In0sICJsZW5ndGgi"
        "OiA2NjM5OSB9LCAiZGF0YWRvZy8yL0ZFQVRVUkVTL2x1a2Uuc3RlZW5zZW4vY29uZmlnIj"
        "ogeyJjdXN0b20iOiB7InYiOiAzIH0sICJoYXNoZXMiOiB7InNoYTI1NiI6ICJjNmE4Y2Q1"
        "MzUzMGI1OTJhNTA5N2EzMjMyY2U0OWNhMDgwNmZhMzI0NzM1NjRjM2FiMzg2YmViYWVhN2"
        "Q4NzQwIn0sICJsZW5ndGgiOiAxMyB9LCAiZW1wbG95ZWUvRkVBVFVSRVMvMi50ZXN0MS5j"
        "b25maWcvY29uZmlnIjogeyJjdXN0b20iOiB7InYiOiAxIH0sICJoYXNoZXMiOiB7InNoYT"
        "I1NiI6ICI0N2VkODI1NjQ3YWQwZWM2YTc4OTkxODg5MDU2NWQ0NGMzOWE1ZTRiYWNkMzVi"
        "YjM0ZjlkZjM4MzM4OTEyZGFlIn0sICJsZW5ndGgiOiA0MSB9IH0sICJ2ZXJzaW9uIjogMj"
        "c0ODcxNTYgfSB9\", \"target_files\": [" +
        target_files +
        "], \"client_configs\": "
        "[\"datadog/2/FEATURES/luke.steensen/config\", "
        "\"employee/FEATURES/2.test1.config/config\"] }");

    return response;
}

remote_config::protocol::get_configs_request generate_request(
    bool generate_state, bool generate_cache)
{
    dds::remote_config::protocol::client protocol_client =
        generate_client(generate_state);
    std::vector<remote_config::protocol::cached_target_files> files;
    if (generate_cache) {
        // First cached file
        std::string hash_key = "sha256";
        std::string hash_value_01 =
            "c6a8cd53530b592a5097a3232ce49ca0806fa32473564c3ab386bebaea7d8740";
        remote_config::protocol::cached_target_files_hash hash01(
            hash_key, hash_value_01);
        std::vector<remote_config::protocol::cached_target_files_hash>
            hashes01 = {hash01};
        std::string path01 = "datadog/2/FEATURES/luke.steensen/config";
        remote_config::protocol::cached_target_files file01(
            path01, 13, hashes01);
        files.push_back(file01);

        // Second cached file
        std::string hash_value_02 =
            "47ed825647ad0ec6a789918890565d44c39a5e4bacd35bb34f9df38338912dae";
        remote_config::protocol::cached_target_files_hash hash02(
            hash_key, hash_value_02);
        std::vector<remote_config::protocol::cached_target_files_hash>
            hashes02 = {hash02};
        std::string path02 = "employee/FEATURES/2.test1.config/config";
        remote_config::protocol::cached_target_files file02(
            path02, 41, hashes02);
        files.push_back(file02);
    }
    remote_config::protocol::get_configs_request request(
        protocol_client, files);

    return request;
}

std::string generate_request_serialized(
    bool generate_state, bool generate_cache)
{
    std::string request_serialized;

    remote_config::protocol::serialize(
        generate_request(generate_state, generate_cache), request_serialized);

    return request_serialized;
}

bool validate_request_has_error(
    std::string request_serialized, bool has_error, std::string error_msg)
{
    rapidjson::Document serialized_doc;
    if (serialized_doc.Parse(request_serialized).HasParseError()) {
        return false;
    }

    rapidjson::Value::ConstMemberIterator state_itr =
        serialized_doc.FindMember("client")->value.FindMember("state");

    // Has error field
    rapidjson::Value::ConstMemberIterator itr =
        state_itr->value.FindMember("has_error");
    rapidjson::Type expected_type =
        has_error ? rapidjson::kTrueType : rapidjson::kFalseType;
    if (itr->value.GetType() != expected_type) {
        return false;
    }

    // Error field
    itr = state_itr->value.FindMember("error");
    if (itr->value.GetType() != rapidjson::kStringType ||
        error_msg != itr->value.GetString()) {
        return false;
    }

    return true;
}

TEST(RemoteConfigClient, ItReturnsErrorIfApiReturnsError)
{
    mock::api *const mock_api = new mock::api;
    EXPECT_CALL(*mock_api, get_configs)
        .WillOnce(Return(remote_config::protocol::remote_config_result::error));

    std::vector<dds::remote_config::product> _products(get_products());
    std::string _runtime_id(runtime_id);
    dds::remote_config::client api_client(mock_api, id, _runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
    delete mock_api;
}

TEST(RemoteConfigClient, ItCallsToApiOnPoll)
{
    // First poll dont have state
    std::string expected_request = generate_request_serialized(false, false);

    mock::api *const mock_api = new mock::api;
    EXPECT_CALL(*mock_api, get_configs(expected_request, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(
            mock::set_response_body(generate_example_response(true)),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    std::string _runtime_id(runtime_id);
    dds::remote_config::client api_client(mock_api, id, _runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    delete mock_api;
}

TEST(RemoteConfigClient, ItReturnErrorWhenApiNotProvided)
{
    std::vector<dds::remote_config::product> _products(get_products());
    std::string _runtime_id(runtime_id);
    dds::remote_config::client api_client(nullptr, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST(RemoteConfigClient, ItReturnErrorWhenResponseIsInvalidJson)
{
    mock::api mock_api;
    EXPECT_CALL(mock_api, get_configs)
        .WillOnce(DoAll(mock::set_response_body("invalid json here"),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(&mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST(RemoteConfigClient,
    ItReturnErrorAndSaveLastErrorWhenClientConfigPathNotInTargetPaths)
{
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiX3R5cGUiOiAidGFyZ2V0cyIsCiAg"
        "ICAgICAgImN1c3RvbSI6IHsKICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIj"
        "ogInNvbWV0aGluZyIKICAgICAgICB9LAogICAgICAgICJ0YXJnZXRzIjogewogICAgICAg"
        "ICAgICAiZGF0YWRvZy8yL0FQTV9TQU1QTElORy9zb21lX290aGVyL2NvbmZpZyI6IHsKIC"
        "AgICAgICAgICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICAgICAgICAgInYiOiAz"
        "Njc0MAogICAgICAgICAgICAgICAgfSwKICAgICAgICAgICAgICAgICJoYXNoZXMiOiB7Ci"
        "AgICAgICAgICAgICAgICAgICAgInNoYTI1NiI6ICIwNzQ2NWNlY2U0N2U0NTQyYWJjMGRh"
        "MDQwZDllYmI0MmVjOTcyMjQ5MjBkNjg3MDY1MWRjMzMxNjUyODYwOWQ1IgogICAgICAgIC"
        "AgICAgICAgfSwKICAgICAgICAgICAgICAgICJsZW5ndGgiOiA2NjM5OQogICAgICAgICAg"
        "ICB9CiAgICAgICAgfSwKICAgICAgICAidmVyc2lvbiI6IDI3NDg3MTU2CiAgICB9Cn0="
        "\", \"target_files\": [], "
        "\"client_configs\": "
        "[\"datadog/2/DEBUG/notfound.insignedtargets/config\"] "
        "}");

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(mock::set_response_body(response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(&mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    // Validate first request does not contain any error
    auto poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, false, ""));

    // Validate second request contains error
    poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, true,
        "missing config datadog/2/DEBUG/notfound.insignedtargets/config in "
        "targets"));
}

TEST(RemoteConfigClient,
    ItReturnErrorAndSaveLastErrorWhenClientConfigPathNotInTargetFiles)
{
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAgIl90eXBlIjogInRhcmdldHMiLAog"
        "ICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICJvcGFxdWVfYmFja2VuZF9zdGF0ZS"
        "I6ICJzb21ldGhpbmciCiAgICAgICAgfSwKICAgICAgICAidGFyZ2V0cyI6IHsKICAgICAg"
        "ICAgICAgImRhdGFkb2cvMi9BUE1fU0FNUExJTkcvZHluYW1pY19yYXRlcy9jb25maWciOi"
        "B7CiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgICAgICJ2"
        "IjogMzY3NDAKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiaGFzaGVzIj"
        "ogewogICAgICAgICAgICAgICAgICAgICJzaGEyNTYiOiAiMDc0NjVjZWNlNDdlNDU0MmFi"
        "YzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZDY4NzA2NTFkYzMzMTY1Mjg2MDlkNSIKICAgIC"
        "AgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAibGVuZ3RoIjogNjYzOTkKICAgICAg"
        "ICAgICAgfQogICAgICAgIH0sCiAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgfQ"
        "p9\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(mock::set_response_body(response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(&mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    // Validate first request does not contain any error
    auto poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, false, ""));

    // Validate second request contains error
    poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, true,
        "missing config datadog/2/APM_SAMPLING/dynamic_rates/config in "
        "target files and in cache files"));
}

TEST(ClientConfig, ItGetGeneratedFromString)
{
    remote_config::config_path cp;
    remote_config::protocol::remote_config_result result;

    result = config_path_from_path(
        "datadog/2/LIVE_DEBUGGING/9e413cda-647b-335b-adcd-7ce453fc2284/config",
        cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    EXPECT_EQ("LIVE_DEBUGGING", cp.get_product());
    EXPECT_EQ("9e413cda-647b-335b-adcd-7ce453fc2284", cp.get_id());

    result =
        config_path_from_path("employee/DEBUG_DD/2.test1.config/config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    EXPECT_EQ("DEBUG_DD", cp.get_product());
    EXPECT_EQ("2.test1.config", cp.get_id());

    result = config_path_from_path(
        "datadog/55/APM_SAMPLING/dynamic_rates/config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    EXPECT_EQ(apm_sampling, cp.get_product());
    EXPECT_EQ("dynamic_rates", cp.get_id());
}

TEST(ClientConfig, ItDoesNotGetGeneratedFromStringIfNotValidMatch)
{
    remote_config::config_path cp;
    remote_config::protocol::remote_config_result result;

    result = config_path_from_path("", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = config_path_from_path("invalid", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = config_path_from_path("datadog/55/APM_SAMPLING/config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = config_path_from_path("datadog/55/APM_SAMPLING//config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = config_path_from_path(
        "datadog/aa/APM_SAMPLING/dynamic_rates/config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = config_path_from_path(
        "something/APM_SAMPLING/dynamic_rates/config", cp);
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST(RemoteConfigClient, ItReturnsErrorWhenClientConfigPathCantBeParsed)
{
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAgIl90eXBlIjogInRhcmdldHMiLAog"
        "ICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICJvcGFxdWVfYmFja2VuZF9zdGF0ZS"
        "I6ICJzb21ldGhpbmciCiAgICAgICAgfSwKICAgICAgICAidGFyZ2V0cyI6IHsKICAgICAg"
        "ICAgICAgImRhdGFkb2cvMi9BUE1fU0FNUExJTkcvZHluYW1pY19yYXRlcy9jb25maWciOi"
        "B7CiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgICAgICJ2"
        "IjogMzY3NDAKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiaGFzaGVzIj"
        "ogewogICAgICAgICAgICAgICAgICAgICJzaGEyNTYiOiAiMDc0NjVjZWNlNDdlNDU0MmFi"
        "YzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZDY4NzA2NTFkYzMzMTY1Mjg2MDlkNSIKICAgIC"
        "AgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAibGVuZ3RoIjogNjYzOTkKICAgICAg"
        "ICAgICAgfQogICAgICAgIH0sCiAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgfQ"
        "p9\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"invalid/path/dynamic_rates/config\"] "
        "}");

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(mock::set_response_body(response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(&mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    // Validate first request does not contain any error
    auto poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, false, ""));

    // Validate second request contains error
    poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, true,
        "error parsing path invalid/path/dynamic_rates/config"));
}

TEST(RemoteConfigClient, ItReturnsErrorIfProductOnPathNotRequested)
{
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAgIl90eXBlIjogInRhcmdldHMiLAog"
        "ICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICJvcGFxdWVfYmFja2VuZF9zdGF0ZS"
        "I6ICJzb21ldGhpbmciCiAgICAgICAgfSwKICAgICAgICAidGFyZ2V0cyI6IHsKICAgICAg"
        "ICAgICAgImRhdGFkb2cvMi9BUE1fU0FNUExJTkcvZHluYW1pY19yYXRlcy9jb25maWciOi"
        "B7CiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgICAgICJ2"
        "IjogMzY3NDAKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiaGFzaGVzIj"
        "ogewogICAgICAgICAgICAgICAgICAgICJzaGEyNTYiOiAiMDc0NjVjZWNlNDdlNDU0MmFi"
        "YzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZDY4NzA2NTFkYzMzMTY1Mjg2MDlkNSIKICAgIC"
        "AgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAibGVuZ3RoIjogNjYzOTkKICAgICAg"
        "ICAgICAgfQogICAgICAgIH0sCiAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgfQ"
        "p9\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(mock::set_response_body(response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<remote_config::product_listener *> listeners;
    remote_config::product p(features, listeners);
    std::vector<remote_config::product> requested_products;
    std::string _runtime_id(runtime_id);
    dds::remote_config::client api_client(&mock_api, id, _runtime_id,
        tracer_version, service, env, app_version, requested_products);

    // Validate first request does not contain any error
    auto poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, false, ""));

    // Validate second request contains error
    poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, true,
        "received config datadog/2/APM_SAMPLING/dynamic_rates/config for a "
        "product that was not requested"));
}

TEST(RemoteConfigClient, ItGeneratesClientStateAndCacheFromResponse)
{
    mock::api *const mock_api = new mock::api;

    // First call should not contain state neither cache
    std::string first_request_no_state_no_cache =
        generate_request_serialized(false, false);
    EXPECT_CALL(*mock_api, get_configs(first_request_no_state_no_cache, _))
        .Times(1)
        .WillOnce(DoAll(
            mock::set_response_body(generate_example_response(true)),
            Return(remote_config::protocol::remote_config_result::success)));

    // Second call. This should contain state and cache from previous response
    std::string second_request_with_state_and_cache =
        generate_request_serialized(true, true);
    EXPECT_CALL(*mock_api, get_configs(second_request_with_state_and_cache, _))
        .Times(1)
        .WillOnce(DoAll(
            mock::set_response_body(generate_example_response(true)),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST(RemoteConfigClient, WhenANewConfigIsAddedItCallsOnUpdateOnPoll)
{
    mock::api *const mock_api = new mock::api;

    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAgIl90eXBlIjogInRhcmdldHMiLAog"
        "ICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICJvcGFxdWVfYmFja2VuZF9zdGF0ZS"
        "I6ICJzb21ldGhpbmciCiAgICAgICAgfSwKICAgICAgICAidGFyZ2V0cyI6IHsKICAgICAg"
        "ICAgICAgImRhdGFkb2cvMi9BUE1fU0FNUExJTkcvZHluYW1pY19yYXRlcy9jb25maWciOi"
        "B7CiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgICAgICJ2"
        "IjogMzY3NDAKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiaGFzaGVzIj"
        "ogewogICAgICAgICAgICAgICAgICAgICJzaGEyNTYiOiAiMDc0NjVjZWNlNDdlNDU0MmFi"
        "YzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZDY4NzA2NTFkYzMzMTY1Mjg2MDlkNSIKICAgIC"
        "AgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAibGVuZ3RoIjogNjYzOTkKICAgICAg"
        "ICAgICAgfQogICAgICAgIH0sCiAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgfQ"
        "p9\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(1)
        .WillOnce(DoAll(mock::set_response_body(response),
            Return(remote_config::protocol::remote_config_result::success)));

    std::string product_str = "APM_SAMPLING";
    std::string id_product = "dynamic_rates";
    std::string path = "datadog/2/APM_SAMPLING/dynamic_rates/config";
    std::string content =
        "UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=";
    std::map<std::string, std::string> hashes = {std::pair<std::string,
        std::string>("sha256",
        "07465cece47e4542abc0da040d9ebb42ec97224920d6870651dc3316528609d5")};
    remote_config::config expected_config(
        product_str, id_product, content, hashes, 36740, path, 66399);
    std::vector<remote_config::config> expected_configs = {expected_config};

    // Product on response
    std::vector<remote_config::config> no_updates;
    mock::listener_mock listener01;
    EXPECT_CALL(listener01, on_update(expected_configs)).Times(1);
    EXPECT_CALL(listener01, on_unapply(no_updates)).Times(1);
    mock::listener_mock listener02;
    EXPECT_CALL(listener02, on_update(expected_configs)).Times(1);
    EXPECT_CALL(listener02, on_unapply(no_updates)).Times(1);
    std::vector<remote_config::product_listener *> listeners = {
        &listener01, &listener02};
    remote_config::product product(apm_sampling, listeners);

    // Product on response
    mock::listener_mock listener_called_no_configs01;
    EXPECT_CALL(listener_called_no_configs01, on_update(no_updates)).Times(1);
    EXPECT_CALL(listener_called_no_configs01, on_unapply(no_updates)).Times(1);
    mock::listener_mock listener_called_no_configs02;
    EXPECT_CALL(listener_called_no_configs02, on_update(no_updates)).Times(1);
    EXPECT_CALL(listener_called_no_configs02, on_unapply(no_updates)).Times(1);
    std::vector<remote_config::product_listener *>
        listeners_called_with_no_configs = {
            &listener_called_no_configs01, &listener_called_no_configs02};
    std::string product_str_not_in_response = "NOT_IN_RESPONSE";
    remote_config::product product_not_in_response(
        product_str_not_in_response, listeners_called_with_no_configs);

    std::vector<dds::remote_config::product> _products = {
        product, product_not_in_response};

    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST(RemoteConfigClient, WhenAConfigDissapearOnFollowingPollsItCallsToUnApply)
{
    mock::api *const mock_api = new mock::api;

    // It contains datadog/2/APM_SAMPLING/dynamic_rates/config
    std::string response01(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAgIl90eXBlIjogInRhcmdldHMiLAog"
        "ICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICJvcGFxdWVfYmFja2VuZF9zdGF0ZS"
        "I6ICJzb21ldGhpbmciCiAgICAgICAgfSwKICAgICAgICAidGFyZ2V0cyI6IHsKICAgICAg"
        "ICAgICAgImRhdGFkb2cvMi9BUE1fU0FNUExJTkcvZHluYW1pY19yYXRlcy9jb25maWciOi"
        "B7CiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgICAgICJ2"
        "IjogMzY3NDAKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiaGFzaGVzIj"
        "ogewogICAgICAgICAgICAgICAgICAgICJzaGEyNTYiOiAiMDc0NjVjZWNlNDdlNDU0MmFi"
        "YzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZDY4NzA2NTFkYzMzMTY1Mjg2MDlkNSIKICAgIC"
        "AgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAibGVuZ3RoIjogNjYzOTkKICAgICAg"
        "ICAgICAgfQogICAgICAgIH0sCiAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgfQ"
        "p9\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    // It does NOT contains datadog/2/APM_SAMPLING/dynamic_rates/config
    std::string response02(
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduZWQiOiB7CiAgICAgICAgICAgICAgICAiX3R5cGUiOiAidGFy"
        "Z2V0cyIsCiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgIC"
        "AgICAgICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgICAg"
        "ICAgICAgfSwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgICAgICAgIC"
        "AgICAgICAgICAiZGF0YWRvZy8yL0FQTV9TQU1QTElORy9hbm90aGVyX29uZS9jb25maWci"
        "OiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgImN1c3RvbSI6IHsKICAgIC"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJ2IjogMzY3NDAKICAgICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICJoYXNoZXMiOiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAic2hhMjU2IjogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZW"
        "M5NzIyNDkyMGQ2ODcwNjUxZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAgICAgfSwKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAibGVuZ3"
        "RoIjogNjYzOTkKICAgICAgICAgICAgICAgICAgICAgICAgfQogICAgICAgICAgICAgICAg"
        "fSwKICAgICAgICAgICAgICAgICJ2ZXJzaW9uIjogMjc0ODcxNTYKICAgICAgICB9Cn0="
        "\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/another_one/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/another_one/config\"] "
        "}");

    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(2)
        .WillOnce(DoAll(mock::set_response_body(response01),
            Return(remote_config::protocol::remote_config_result::success)))
        .WillOnce(DoAll(mock::set_response_body(response02),
            Return(remote_config::protocol::remote_config_result::success)));

    std::string product_str = "APM_SAMPLING";
    std::string id_product = "dynamic_rates";
    std::string path_01 = "datadog/2/APM_SAMPLING/dynamic_rates/config";
    std::string content =
        "UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=";
    std::map<std::string, std::string> hashes = {std::pair<std::string,
        std::string>("sha256",
        "07465cece47e4542abc0da040d9ebb42ec97224920d6870651dc3316528609d5")};
    remote_config::config expected_config(
        product_str, id_product, content, hashes, 36740, path_01, 66399);
    std::vector<remote_config::config> expected_configs_first_call = {
        expected_config};
    std::vector<remote_config::config> no_updates;

    std::string id_product_02 = "another_one";
    std::string path_02 = "datadog/2/APM_SAMPLING/another_one/config";
    remote_config::config expected_config_02(
        product_str, id_product_02, content, hashes, 36740, path_02, 66399);
    std::vector<remote_config::config> expected_configs_second_call = {
        expected_config_02};

    // Product on response
    mock::listener_mock listener01;
    // Second poll expectations
    EXPECT_CALL(listener01, on_update(expected_configs_second_call))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(expected_configs_first_call))
        .Times(1)
        .RetiresOnSaturation();
    // First poll expectations
    EXPECT_CALL(listener01, on_update(expected_configs_first_call))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(no_updates))
        .Times(1)
        .RetiresOnSaturation();
    std::vector<remote_config::product_listener *> listeners = {&listener01};
    remote_config::product product(apm_sampling, listeners);

    std::vector<dds::remote_config::product> _products = {product};

    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST(
    RemoteConfigClient, WhenAConfigGetsUpdatedOnFollowingPollsItCallsToUnUpdate)
{
    mock::api *const mock_api = new mock::api;

    std::string response01(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiX3R5cGUiOiAidGFyZ2V0cyIsCiAg"
        "ICAgICAgImN1c3RvbSI6IHsKICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIj"
        "ogInNvbWV0aGluZyIKICAgICAgICB9LAogICAgICAgICJ0YXJnZXRzIjogewogICAgICAg"
        "ICAgICAiZGF0YWRvZy8yL0FQTV9TQU1QTElORy9keW5hbWljX3JhdGVzL2NvbmZpZyI6IH"
        "sKICAgICAgICAgICAgICAgICJjdXN0b20iOiB7CiAgICAgICAgICAgICAgICAgICAgInYi"
        "OiAzNjc0MAogICAgICAgICAgICAgICAgfSwKICAgICAgICAgICAgICAgICJoYXNoZXMiOi"
        "B7CiAgICAgICAgICAgICAgICAgICAgInNoYTI1NiI6ICIwNzQ2NWNlY2U0N2U0NTQyYWJj"
        "MGRhMDQwZDllYmI0MmVjOTcyMjQ5MjBkNjg3MDY1MWRjMzMxNjUyODYwOWQ1IgogICAgIC"
        "AgICAgICAgICAgfSwKICAgICAgICAgICAgICAgICJsZW5ndGgiOiA2NjM5OQogICAgICAg"
        "ICAgICB9CiAgICAgICAgfSwKICAgICAgICAidmVyc2lvbiI6IDI3NDg3MTU2CiAgICB9Cn"
        "0=\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    std::string response02(
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduZWQiOiB7CiAgICAgICAgICAgICAgICAiX3R5cGUiOiAidGFy"
        "Z2V0cyIsCiAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAgICAgICAgIC"
        "AgICAgICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgICAg"
        "ICAgICAgfSwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgICAgICAgIC"
        "AgICAgICAgICAiZGF0YWRvZy8yL0FQTV9TQU1QTElORy9keW5hbWljX3JhdGVzL2NvbmZp"
        "ZyI6IHsKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAiY3VzdG9tIjogewogIC"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgInYiOiAzNjc0MAogICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAgICJzaGEyNTYiOiAiYW5vdGhlcl9oYXNoX2hlcmUiCiAgICAgICAgICAgIC"
        "AgICAgICAgICAgICAgICAgICAgfSwKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAibGVuZ3RoIjogNjYzOTkKICAgICAgICAgICAgICAgICAgICAgICAgfQogICAgICAgIC"
        "AgICAgICAgfSwKICAgICAgICAgICAgICAgICJ2ZXJzaW9uIjogMjc0ODcxNTYKICAgICAg"
        "ICB9Cn0=\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(2)
        .WillOnce(DoAll(mock::set_response_body(response01),
            Return(remote_config::protocol::remote_config_result::success)))
        .WillOnce(DoAll(mock::set_response_body(response02),
            Return(remote_config::protocol::remote_config_result::success)));

    std::string product_str = "APM_SAMPLING";
    std::string id_product = "dynamic_rates";
    std::string path = "datadog/2/APM_SAMPLING/dynamic_rates/config";
    std::string content =
        "UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=";
    std::map<std::string, std::string> hashes_01 = {std::pair<std::string,
        std::string>("sha256",
        "07465cece47e4542abc0da040d9ebb42ec97224920d6870651dc3316528609d5")};
    remote_config::config expected_config(
        product_str, id_product, content, hashes_01, 36740, path, 66399);
    std::vector<remote_config::config> expected_configs_first_call = {
        expected_config};
    std::vector<remote_config::config> no_updates;

    std::map<std::string, std::string> hashes_02 = {
        std::pair<std::string, std::string>("sha256", "another_hash_here")};
    remote_config::config expected_config_02(
        product_str, id_product, content, hashes_02, 36740, path, 66399);
    std::vector<remote_config::config> expected_configs_second_call = {
        expected_config_02};

    // Product on response
    mock::listener_mock listener01;
    // Second poll expectations
    EXPECT_CALL(listener01, on_update(expected_configs_second_call))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(no_updates))
        .Times(1)
        .RetiresOnSaturation();
    // First poll expectations
    EXPECT_CALL(listener01, on_update(expected_configs_first_call))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(no_updates))
        .Times(1)
        .RetiresOnSaturation();
    std::vector<remote_config::product_listener *> listeners = {&listener01};
    remote_config::product product(apm_sampling, listeners);

    std::vector<dds::remote_config::product> _products = {product};

    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST(RemoteConfigClient, FilesThatAreInCacheAreUsedWhenNotInTargetFiles)
{
    mock::api *const mock_api = new mock::api;

    // First call should not contain state neither cache
    std::string first_request_no_state_no_cache =
        generate_request_serialized(false, false);
    EXPECT_CALL(*mock_api, get_configs(first_request_no_state_no_cache, _))
        .Times(1)
        .WillOnce(
            DoAll(mock::set_response_body(generate_example_response(true)),
                Return(remote_config::protocol::remote_config_result::success)))
        .RetiresOnSaturation();

    // Second call. Since this call has cache, response comes without
    // target_files
    // Third call. Cache and state should be keeped even though
    // target_files came empty on second
    std::string second_request_with_state_and_cache =
        generate_request_serialized(true, true);
    EXPECT_CALL(*mock_api, get_configs(second_request_with_state_and_cache, _))
        .Times(2)
        .WillOnce(
            DoAll(mock::set_response_body(generate_example_response(false)),
                Return(remote_config::protocol::remote_config_result::success)))
        .WillOnce(DoAll(
            mock::set_response_body(generate_example_response(false)),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST(RemoteConfigClient, NotTrackedFilesAreDeletedFromCache)
{
    mock::api *const mock_api = new mock::api;

    std::string request_sent;
    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(3)
        .WillOnce(
            DoAll(mock::set_response_body(generate_example_response(true)),
                Return(remote_config::protocol::remote_config_result::success)))
        .WillOnce(DoAll(mock::set_response_body(generate_empty_response()),
            Return(remote_config::protocol::remote_config_result::success)))
        .WillOnce(DoAll(testing::SaveArg<0>(&request_sent),
            mock::set_response_body(generate_empty_response()),
            Return(remote_config::protocol::remote_config_result::success)));

    std::vector<dds::remote_config::product> _products = {get_products()};

    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    // Lets validate cached_target_files is empty
    rapidjson::Document serialized_doc;
    serialized_doc.Parse(request_sent);
    auto output_itr = serialized_doc.FindMember("cached_target_files");

    EXPECT_FALSE(output_itr == serialized_doc.MemberEnd());
    EXPECT_TRUE(rapidjson::kArrayType == output_itr->value.GetType());
    EXPECT_EQ(0, output_itr->value.GetArray().Size());

    delete mock_api;
}

TEST(RemoteConfigClient, TestHashIsDifferentFromTheCache)
{
    mock::api *const mock_api = new mock::api;

    std::string first_response =
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduYXR1cmVzIjogWwogICAgICAgICAgICAgICAgewogICAgICAg"
        "ICAgICAgICAgICAgICAgICAia2V5aWQiOiAiNWM0ZWNlNDEyNDFhMWJiNTEzZjZlM2U1ZG"
        "Y3NGFiN2Q1MTgzZGZmZmJkNzFiZmQ0MzEyNzkyMGQ4ODA1NjlmZCIsCiAgICAgICAgICAg"
        "ICAgICAgICAgICAgICJzaWciOiAiNDliOTBmNWY0YmZjMjdjY2JkODBkOWM4NDU4ZDdkMj"
        "JiYTlmYTA4OTBmZDc3NWRkMTE2YzUyOGIzNmRkNjA1YjFkZjc2MWI4N2I2YzBlYjliMDI2"
        "NDA1YTEzZWZlZjQ4Mjc5MzRkNmMyNWE3ZDZiODkyNWZkYTg5MjU4MDkwMGYiCiAgICAgIC"
        "AgICAgICAgICB9CiAgICAgICAgXSwKICAgICAgICAic2lnbmVkIjogewogICAgICAgICAg"
        "ICAgICAgIl90eXBlIjogInRhcmdldHMiLAogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIjogInNv"
        "bWV0aGluZyIKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiZXhwaXJlcy"
        "I6ICIyMDIyLTExLTA0VDEzOjMxOjU5WiIsCiAgICAgICAgICAgICAgICAic3BlY192ZXJz"
        "aW9uIjogIjEuMC4wIiwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgIC"
        "AgICAgICAgICAgICAgICAiZW1wbG95ZWUvRkVBVFVSRVMvMi50ZXN0MS5jb25maWcvY29u"
        "ZmlnIjogewogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJjdXN0b20iOiB7Ci"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAidiI6IDEKICAgICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICJoYXNoZXMiOiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAic2hhMjU2IjogInNvbWVfaGFzaCIKICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJsZW5ndGgi"
        "OiA0MQogICAgICAgICAgICAgICAgICAgICAgICB9CiAgICAgICAgICAgICAgICB9LAogIC"
        "AgICAgICAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgICAgIH0KfQ==\", "
        "\"target_files\": [{\"path\": "
        "\"employee/FEATURES/2.test1.config/config\", \"raw\": \"some_raw=\"} "
        "], \"client_configs\": [\"employee/FEATURES/2.test1.config/config\"] "
        "}";

    // This response has a cached file with different hash, it should not be
    // used
    std::string second_response =
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduYXR1cmVzIjogWwogICAgICAgICAgICAgICAgewogICAgICAg"
        "ICAgICAgICAgICAgICAgICAia2V5aWQiOiAiNWM0ZWNlNDEyNDFhMWJiNTEzZjZlM2U1ZG"
        "Y3NGFiN2Q1MTgzZGZmZmJkNzFiZmQ0MzEyNzkyMGQ4ODA1NjlmZCIsCiAgICAgICAgICAg"
        "ICAgICAgICAgICAgICJzaWciOiAiNDliOTBmNWY0YmZjMjdjY2JkODBkOWM4NDU4ZDdkMj"
        "JiYTlmYTA4OTBmZDc3NWRkMTE2YzUyOGIzNmRkNjA1YjFkZjc2MWI4N2I2YzBlYjliMDI2"
        "NDA1YTEzZWZlZjQ4Mjc5MzRkNmMyNWE3ZDZiODkyNWZkYTg5MjU4MDkwMGYiCiAgICAgIC"
        "AgICAgICAgICB9CiAgICAgICAgXSwKICAgICAgICAic2lnbmVkIjogewogICAgICAgICAg"
        "ICAgICAgIl90eXBlIjogInRhcmdldHMiLAogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIjogInNv"
        "bWV0aGluZyIKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiZXhwaXJlcy"
        "I6ICIyMDIyLTExLTA0VDEzOjMxOjU5WiIsCiAgICAgICAgICAgICAgICAic3BlY192ZXJz"
        "aW9uIjogIjEuMC4wIiwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgIC"
        "AgICAgICAgICAgICAgICAiZW1wbG95ZWUvRkVBVFVSRVMvMi50ZXN0MS5jb25maWcvY29u"
        "ZmlnIjogewogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJjdXN0b20iOiB7Ci"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAidiI6IDEKICAgICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICJoYXNoZXMiOiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAic2hhMjU2IjogInNvbWVfb3RoZXJfaGFzaCIKICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJs"
        "ZW5ndGgiOiA0MQogICAgICAgICAgICAgICAgICAgICAgICB9CiAgICAgICAgICAgICAgIC"
        "B9LAogICAgICAgICAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgICAgIH0KfQ=="
        "\", \"target_files\": [], \"client_configs\": "
        "[\"employee/FEATURES/2.test1.config/config\"] }";

    std::string request_sent;
    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(3)
        .WillOnce(DoAll(mock::set_response_body(first_response),
            Return(remote_config::protocol::remote_config_result::success)))
        .WillRepeatedly(DoAll(mock::set_response_body(second_response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)))
        .RetiresOnSaturation();

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);

    EXPECT_TRUE(validate_request_has_error(request_sent, true,
        "missing config employee/FEATURES/2.test1.config/config in "
        "target files and in cache files"));

    delete mock_api;
}

TEST(RemoteConfigClient, TestWhenFileGetsFromCacheItsCachedLenUsed)
{
    mock::api *const mock_api = new mock::api;

    std::string first_response =
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduYXR1cmVzIjogWwogICAgICAgICAgICAgICAgewogICAgICAg"
        "ICAgICAgICAgICAgICAgICAia2V5aWQiOiAiNWM0ZWNlNDEyNDFhMWJiNTEzZjZlM2U1ZG"
        "Y3NGFiN2Q1MTgzZGZmZmJkNzFiZmQ0MzEyNzkyMGQ4ODA1NjlmZCIsCiAgICAgICAgICAg"
        "ICAgICAgICAgICAgICJzaWciOiAiNDliOTBmNWY0YmZjMjdjY2JkODBkOWM4NDU4ZDdkMj"
        "JiYTlmYTA4OTBmZDc3NWRkMTE2YzUyOGIzNmRkNjA1YjFkZjc2MWI4N2I2YzBlYjliMDI2"
        "NDA1YTEzZWZlZjQ4Mjc5MzRkNmMyNWE3ZDZiODkyNWZkYTg5MjU4MDkwMGYiCiAgICAgIC"
        "AgICAgICAgICB9CiAgICAgICAgXSwKICAgICAgICAic2lnbmVkIjogewogICAgICAgICAg"
        "ICAgICAgIl90eXBlIjogInRhcmdldHMiLAogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIjogInNv"
        "bWV0aGluZyIKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiZXhwaXJlcy"
        "I6ICIyMDIyLTExLTA0VDEzOjMxOjU5WiIsCiAgICAgICAgICAgICAgICAic3BlY192ZXJz"
        "aW9uIjogIjEuMC4wIiwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgIC"
        "AgICAgICAgICAgICAgICAiZW1wbG95ZWUvRkVBVFVSRVMvMi50ZXN0MS5jb25maWcvY29u"
        "ZmlnIjogewogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJjdXN0b20iOiB7Ci"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAidiI6IDEKICAgICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICJoYXNoZXMiOiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAic2hhMjU2IjogInNvbWVfaGFzaCIKICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJsZW5ndGgi"
        "OiA0MQogICAgICAgICAgICAgICAgICAgICAgICB9CiAgICAgICAgICAgICAgICB9LAogIC"
        "AgICAgICAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgICAgIH0KfQ==\", "
        "\"target_files\": [{\"path\": "
        "\"employee/FEATURES/2.test1.config/config\", \"raw\": \"some_raw=\"} "
        "], \"client_configs\": [\"employee/FEATURES/2.test1.config/config\"] "
        "}";

    // This response has a cached file with different len and version, it should
    // not be used
    std::string second_response =
        "{\"roots\": [], \"targets\": "
        "\"ewogICAgICAgICJzaWduYXR1cmVzIjogWwogICAgICAgICAgICAgICAgewogICAgICAg"
        "ICAgICAgICAgICAgICAgICAia2V5aWQiOiAiNWM0ZWNlNDEyNDFhMWJiNTEzZjZlM2U1ZG"
        "Y3NGFiN2Q1MTgzZGZmZmJkNzFiZmQ0MzEyNzkyMGQ4ODA1NjlmZCIsCiAgICAgICAgICAg"
        "ICAgICAgICAgICAgICJzaWciOiAiNDliOTBmNWY0YmZjMjdjY2JkODBkOWM4NDU4ZDdkMj"
        "JiYTlmYTA4OTBmZDc3NWRkMTE2YzUyOGIzNmRkNjA1YjFkZjc2MWI4N2I2YzBlYjliMDI2"
        "NDA1YTEzZWZlZjQ4Mjc5MzRkNmMyNWE3ZDZiODkyNWZkYTg5MjU4MDkwMGYiCiAgICAgIC"
        "AgICAgICAgICB9CiAgICAgICAgXSwKICAgICAgICAic2lnbmVkIjogewogICAgICAgICAg"
        "ICAgICAgIl90eXBlIjogInRhcmdldHMiLAogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAgICAgIm9wYXF1ZV9iYWNrZW5kX3N0YXRlIjogInNv"
        "bWV0aGluZyIKICAgICAgICAgICAgICAgIH0sCiAgICAgICAgICAgICAgICAiZXhwaXJlcy"
        "I6ICIyMDIyLTExLTA0VDEzOjMxOjU5WiIsCiAgICAgICAgICAgICAgICAic3BlY192ZXJz"
        "aW9uIjogIjEuMC4wIiwKICAgICAgICAgICAgICAgICJ0YXJnZXRzIjogewogICAgICAgIC"
        "AgICAgICAgICAgICAgICAiZW1wbG95ZWUvRkVBVFVSRVMvMi50ZXN0MS5jb25maWcvY29u"
        "ZmlnIjogewogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJjdXN0b20iOiB7Ci"
        "AgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAidiI6IDQKICAgICAg"
        "ICAgICAgICAgICAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICJoYXNoZXMiOiB7CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
        "ICAgICAgICAic2hhMjU2IjogInNvbWVfaGFzaCIKICAgICAgICAgICAgICAgICAgICAgIC"
        "AgICAgICAgICB9LAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICJsZW5ndGgi"
        "OiA1NQogICAgICAgICAgICAgICAgICAgICAgICB9CiAgICAgICAgICAgICAgICB9LAogIC"
        "AgICAgICAgICAgICAgInZlcnNpb24iOiAyNzQ4NzE1NgogICAgICAgIH0KfQ==\", "
        "\"target_files\": [], \"client_configs\": "
        "[\"employee/FEATURES/2.test1.config/config\"] }";

    std::string request_sent;
    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(3)
        .WillOnce(DoAll(mock::set_response_body(first_response),
            Return(remote_config::protocol::remote_config_result::success)))
        .WillRepeatedly(DoAll(mock::set_response_body(second_response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)))
        .RetiresOnSaturation();

    std::vector<dds::remote_config::product> _products(get_products());
    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    // Lets validate cached_target_files is empty
    rapidjson::Document serialized_doc;
    serialized_doc.Parse(request_sent);
    auto output_itr = serialized_doc.FindMember("cached_target_files");

    auto files_cached = output_itr->value.GetArray();
    EXPECT_FALSE(output_itr == serialized_doc.MemberEnd());
    EXPECT_TRUE(rapidjson::kArrayType == output_itr->value.GetType());
    EXPECT_EQ(1, files_cached.Size());

    auto len_itr = files_cached[0].FindMember("length");
    EXPECT_FALSE(len_itr == files_cached[0].MemberEnd());
    EXPECT_TRUE(rapidjson::kNumberType == len_itr->value.GetType());
    EXPECT_EQ(41, len_itr->value.GetInt());

    delete mock_api;
}

/*
TEST(RemoteConfigClient, TestAgainstDocker)
{
    dds::cout_listener *listener = new dds::cout_listener();
    std::vector<remote_config::product_listener *> listeners = {
(remote_config::product_listener *)listener}; remote_config::product
product(asm_dd, listeners);

    std::vector<dds::remote_config::product> _products = {product};

    remote_config::http_api api;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto current_id = oss.str();

    dds::remote_config::client api_client(&api, current_id, runtime_id,
tracer_version, service, env, app_version, _products);

    std::cout << "First poll" << std::endl;
    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    sleep(6);
    std::cout << "Second poll" << std::endl;
    result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
}
*/

} // namespace dds
