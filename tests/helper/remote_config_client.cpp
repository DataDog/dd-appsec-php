// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <iostream>
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
std::vector<std::string> products = {"ASM_DD", "FEATURES"};

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
        std::string product00("FEATURES");
        remote_config::protocol::config_state cs00(id00, 3, product00);
        std::string id01 = "2.test1.config";
        std::string product01("FEATURES");
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

std::string generate_example_response()
{
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
        "c0ODcxNTYgfSB9\", \"target_files\": [{\"path\": "
        "\"employee/FEATURES/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{\"path\": \"datadog/2/FEATURES/luke.steensen/config\", \"raw\": "
        "\"aGVsbG8gdmVjdG9yIQ==\"} ], \"client_configs\": "
        "[\"datadog/2/FEATURES/luke.steensen/config\", "
        "\"employee/FEATURES/2.test1.config/config\"] }");

    return response;
}

remote_config::protocol::get_configs_request generate_request(
    bool generate_state)
{
    dds::remote_config::protocol::client protocol_client =
        generate_client(generate_state);
    std::vector<remote_config::protocol::cached_target_files> files;
    remote_config::protocol::get_configs_request request(
        protocol_client, files);

    return request;
}

std::string generate_request_serialized(bool generate_state)
{
    std::string request_serialized;

    remote_config::protocol::serialize(
        generate_request(generate_state), request_serialized);

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
    std::string expected_request = generate_request_serialized(false);

    mock::api *const mock_api = new mock::api;
    EXPECT_CALL(*mock_api, get_configs(expected_request, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(mock::set_response_body(generate_example_response()),
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
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L3NvbWVfb3RoZXIvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IHsKIC"
        "AgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAogICAg"
        "ICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMjU2Ij"
        "ogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcwNjUx"
        "ZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgIm"
        "xlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2ZXJz"
        "aW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\", \"target_files\": [], "
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
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L2R5bmFtaWNfcmF0ZXMvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAog"
        "ICAgICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMj"
        "U2IjogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcw"
        "NjUxZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgIC"
        "AgImxlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2"
        "ZXJzaW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\", \"target_files\": [{\"path\": "
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
        "target files"));
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
    EXPECT_EQ("APM_SAMPLING", cp.get_product());
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
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L2R5bmFtaWNfcmF0ZXMvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAog"
        "ICAgICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMj"
        "U2IjogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcw"
        "NjUxZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgIC"
        "AgImxlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2"
        "ZXJzaW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\", \"target_files\": [{\"path\": "
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
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L2R5bmFtaWNfcmF0ZXMvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAog"
        "ICAgICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMj"
        "U2IjogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcw"
        "NjUxZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgIC"
        "AgImxlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2"
        "ZXJzaW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\", \"target_files\": [{\"path\": "
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
    remote_config::product p("FEATURES", listeners);
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

TEST(RemoteConfigClient, ItGeneratesClientStateFromResponse)
{
    mock::api *const mock_api = new mock::api;

    // First call should not contain state
    std::string first_request_no_state = generate_request_serialized(false);
    EXPECT_CALL(*mock_api, get_configs(first_request_no_state, _))
        .Times(1)
        .WillOnce(DoAll(mock::set_response_body(generate_example_response()),
            Return(remote_config::protocol::remote_config_result::success)));

    // Second call. This should contain state from previous response
    std::string second_request_with_state = generate_request_serialized(true);
    EXPECT_CALL(*mock_api, get_configs(second_request_with_state, _))
        .Times(1)
        .WillOnce(DoAll(mock::set_response_body(generate_example_response()),
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

TEST(RemoteConfigClient, ItCallProductListenersOfUpdatedProduct)
{
    mock::api *const mock_api = new mock::api;

    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L2R5bmFtaWNfcmF0ZXMvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IH"
        "sKICAgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAog"
        "ICAgICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMj"
        "U2IjogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcw"
        "NjUxZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgIC"
        "AgImxlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2"
        "ZXJzaW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\", \"target_files\": [{\"path\": "
        "\"datadog/2/APM_SAMPLING/dynamic_rates/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"} ], "
        "\"client_configs\": [\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    EXPECT_CALL(*mock_api, get_configs(_, _))
        .Times(1)
        .WillOnce(DoAll(mock::set_response_body(response),
            Return(remote_config::protocol::remote_config_result::success)));

    // Product on response
    mock::listener_mock listener01;
    EXPECT_CALL(listener01, on_update(_)).Times(1);
    mock::listener_mock listener02;
    EXPECT_CALL(listener02, on_update(_)).Times(1);
    std::vector<remote_config::product_listener *> listeners = {
        &listener01, &listener02};
    remote_config::product product("APM_SAMPLING", listeners);

    // Product on response
    mock::listener_mock listener_not_called01;
    EXPECT_CALL(listener_not_called01, on_update(_)).Times(0);
    mock::listener_mock listener_not_called02;
    EXPECT_CALL(listener_not_called02, on_update(_)).Times(0);
    std::vector<remote_config::product_listener *> listeners_not_called = {
        &listener_not_called01, &listener_not_called02};
    remote_config::product product_not_in_response(
        "NOT_IN_RESPONSE", listeners);

    std::vector<dds::remote_config::product> _products = {
        product, product_not_in_response};

    dds::remote_config::client api_client(mock_api, id, runtime_id,
        tracer_version, service, env, app_version, _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

} // namespace dds
