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
} // namespace mock

std::string id = "some id";
std::string runtime_id = "some runtime id";
std::string tracer_version = "some tracer version";
std::string service = "some service";
std::string env = "some env";
std::string app_version = "some app version";
std::string backend_client_state = "";
int target_version = 0;
std::vector<remote_config::protocol::product> products = {
    remote_config::protocol::product::asm_dd,
    remote_config::protocol::product::features};

remote_config::protocol::client generate_client()
{
    remote_config::protocol::client_tracer client_tracer(runtime_id.c_str(),
        tracer_version.c_str(), service.c_str(), env.c_str(),
        app_version.c_str());

    std::vector<remote_config::protocol::config_state> config_states;
    remote_config::protocol::client_state client_state(target_version,
        std::move(config_states), false, "", backend_client_state.c_str());

    std::string client_id(id);
    std::vector<remote_config::protocol::product> product_copy(products);
    remote_config::protocol::client client(std::move(client_id),
        std::move(product_copy), client_tracer, client_state);

    return client;
}

std::string generate_example_response()
{
    std::string response(
        "{\"roots\": [], \"targets\": "
        "\"eyJzaWduYXR1cmVzIjpbeyJrZXlpZCI6IjVjNGVjZTQxMjQxYTFiYjUxM2Y2ZTNlNWRm"
        "NzRhYjdkNTE4M2RmZmZiZDcxYmZkNDMxMjc5MjBkODgwNTY5ZmQiLCJzaWciOiI0OWI5MG"
        "Y1ZjRiZmMyN2NjYmQ4MGQ5Yzg0NThkN2QyMmJhOWZhMDg5MGZkNzc1ZGQxMTZjNTI4YjM2"
        "ZGQ2MDViMWRmNzYxYjg3YjZjMGViOWIwMjY0MDVhMTNlZmVmNDgyNzkzNGQ2YzI1YTdkNm"
        "I4OTI1ZmRhODkyNTgwOTAwZiJ9XSwic2lnbmVkIjp7Il90eXBlIjoidGFyZ2V0cyIsImN1"
        "c3RvbSI6eyJvcGFxdWVfYmFja2VuZF9zdGF0ZSI6ImV5SjJaWEp6YVc5dUlqb3hMQ0p6ZE"
        "dGMFpTSTZleUptYVd4bFgyaGhjMmhsY3lJNld5SlNLekpEVm10bGRFUnpZVzVwV2tkSmEw"
        "WmFaRkpOVDJGWWEzVnpNREYxZWxRMU0zcG5lbWxTVEdFMFBTSXNJa0l3V21NM1QxSXJVbF"
        "ZMY25kT2IwVkVXalkzVVhWNVdFbHJhMmN4YjJOSFZXUjNla1pzUzBkRFpGVTlJaXdpZUhG"
        "cVRsVXhUVXhYVTNCUmJEWk5ha3hQVTJOdlNVSjJiM2xTZWxacmR6WnpOR0VyZFhWd09XZ3"
        "dRVDBpWFgxOSJ9LCJleHBpcmVzIjoiMjAyMi0xMS0wNFQxMzozMTo1OVoiLCJzcGVjX3Zl"
        "cnNpb24iOiIxLjAuMCIsInRhcmdldHMiOnsiZGF0YWRvZy8yL0FQTV9TQU1QTElORy9keW"
        "5hbWljX3JhdGVzL2NvbmZpZyI6eyJjdXN0b20iOnsidiI6MzY3NDB9LCJoYXNoZXMiOnsi"
        "c2hhMjU2IjoiMDc0NjVjZWNlNDdlNDU0MmFiYzBkYTA0MGQ5ZWJiNDJlYzk3MjI0OTIwZD"
        "Y4NzA2NTFkYzMzMTY1Mjg2MDlkNSJ9LCJsZW5ndGgiOjY2Mzk5fSwiZGF0YWRvZy8yL0RF"
        "QlVHL2x1a2Uuc3RlZW5zZW4vY29uZmlnIjp7ImN1c3RvbSI6eyJ2IjozfSwiaGFzaGVzIj"
        "p7InNoYTI1NiI6ImM2YThjZDUzNTMwYjU5MmE1MDk3YTMyMzJjZTQ5Y2EwODA2ZmEzMjQ3"
        "MzU2NGMzYWIzODZiZWJhZWE3ZDg3NDAifSwibGVuZ3RoIjoxM30sImVtcGxveWVlL0RFQl"
        "VHX0RELzIudGVzdDEuY29uZmlnL2NvbmZpZyI6eyJjdXN0b20iOnsidiI6MX0sImhhc2hl"
        "cyI6eyJzaGEyNTYiOiI0N2VkODI1NjQ3YWQwZWM2YTc4OTkxODg5MDU2NWQ0NGMzOWE1ZT"
        "RiYWNkMzViYjM0ZjlkZjM4MzM4OTEyZGFlIn0sImxlbmd0aCI6NDF9fSwidmVyc2lvbiI6"
        "Mjc0ODcxNTZ9fQ==\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{\"path\": \"datadog/2/DEBUG/luke.steensen/config\", \"raw\": "
        "\"aGVsbG8gdmVjdG9yIQ==\"} ], \"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");

    return response;
}

remote_config::protocol::get_configs_request generate_request()
{
    dds::remote_config::protocol::client protocol_client = generate_client();
    std::vector<remote_config::protocol::cached_target_files> files;
    remote_config::protocol::get_configs_request request(
        protocol_client, std::move(files));

    return request;
}

std::string generate_request_serialized()
{
    std::string request_serialized;

    remote_config::protocol::serialize(generate_request(), request_serialized);

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

    dds::remote_config::client api_client(mock_api, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products));

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
    delete mock_api;
}

TEST(RemoteConfigClient, ItCallsToApiOnPoll)
{
    std::string expected_request = generate_request_serialized();

    mock::api *const mock_api = new mock::api;
    EXPECT_CALL(*mock_api, get_configs(expected_request, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(mock::set_response_body(generate_example_response()),
            Return(remote_config::protocol::remote_config_result::success)));

    dds::remote_config::client api_client(mock_api, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products));

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    delete mock_api;
}

TEST(RemoteConfigClient, ItReturnErrorWhenApiNotProvided)
{
    dds::remote_config::client api_client(nullptr, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products));

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST(RemoteConfigClient, ItReturnErrorWhenResponseIsInvalidJson)
{
    mock::api mock_api;
    EXPECT_CALL(mock_api, get_configs)
        .WillOnce(DoAll(mock::set_response_body("invalid json here"),
            Return(remote_config::protocol::remote_config_result::success)));

    dds::remote_config::client api_client(&mock_api, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products));

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST(RemoteConfigClient,
    ItReturnErrorAndSaveLastErrorWhenClientConfigPathNotInTargetPaths)
{
    std::string response(
        "{\n"
        "    \"roots\": [],\n"
        "    \"targets\": "
        "\"eyAgIAogICAgInNpZ25lZCI6IHsKICAgICAgICAiY3VzdG9tIjogewogICAgICAgICAg"
        "ICAib3BhcXVlX2JhY2tlbmRfc3RhdGUiOiAic29tZXRoaW5nIgogICAgICAgIH0sCiAgIC"
        "AgICAgInRhcmdldHMiOiB7CiAgICAgICAgICAgICJkYXRhZG9nLzIvQVBNX1NBTVBMSU5H"
        "L3NvbWVfb3RoZXIvY29uZmlnIjogewogICAgICAgICAgICAgICAgImN1c3RvbSI6IHsKIC"
        "AgICAgICAgICAgICAgICAgICAidiI6IDM2NzQwCiAgICAgICAgICAgICAgICB9LAogICAg"
        "ICAgICAgICAgICAgImhhc2hlcyI6IHsKICAgICAgICAgICAgICAgICAgICAic2hhMjU2Ij"
        "ogIjA3NDY1Y2VjZTQ3ZTQ1NDJhYmMwZGEwNDBkOWViYjQyZWM5NzIyNDkyMGQ2ODcwNjUx"
        "ZGMzMzE2NTI4NjA5ZDUiCiAgICAgICAgICAgICAgICB9LAogICAgICAgICAgICAgICAgIm"
        "xlbmd0aCI6IDY2Mzk5CiAgICAgICAgICAgIH0KICAgICAgICB9LAogICAgICAgICJ2ZXJz"
        "aW9uIjogMjc0ODcxNTYKICAgIH0KfQ==\",\n"
        "    \"target_files\": [],\n"
        "    \"client_configs\": [\n"
        "        \"datadog/2/DEBUG/notfound.insignedtargets/config\"\n"
        "    ]\n"
        "}");

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(mock::set_response_body(response),
            testing::SaveArg<0>(&request_sent),
            Return(remote_config::protocol::remote_config_result::success)));

    dds::remote_config::client api_client(&mock_api, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products));

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

} // namespace dds
