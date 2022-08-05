// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "remote_config/protocol/tuf/get_configs_response.hpp"
#include "remote_config/protocol/tuf/parser.hpp"

namespace dds {

std::string get_example_response()
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

TEST(RemoteConfigParser, ItReturnsErrorWhenInvalidBodyIsGiven)
{
    std::string response("invalid_json");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::invalid_json, result);
}

TEST(RemoteConfigParser, TargetsFieldIsRequired)
{
    std::string response("{\"target_files\": [], \"client_configs\": [] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::targets_field_missing,
        result);
}

TEST(RemoteConfigParser, TargetsFieldMustBeString)
{
    std::string response(
        "{\"targets\": [], \"target_files\": [], \"client_configs\": [] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(
        remote_config::remote_config_parser_result::targets_field_invalid_type,
        result);
}

TEST(RemoteConfigParser, targetFilesFieldIsRequired)
{
    std::string response("{\"targets\": \"\", \"client_configs\": [] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(
        remote_config::remote_config_parser_result::target_files_field_missing,
        result);
}

TEST(RemoteConfigParser, targetFilesFieldMustBeArray)
{
    std::string response(
        "{\"targets\": \"\", \"target_files\": \"\", \"client_configs\": [] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  target_files_field_invalid_type,
        result);
}

TEST(RemoteConfigParser, clientConfigsFieldIsRequired)
{
    std::string response("{\"targets\": \"\", \"target_files\": [] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(
        remote_config::remote_config_parser_result::client_config_field_missing,
        result);
}

TEST(RemoteConfigParser, clientConfigsFieldMustBeArray)
{
    std::string response(
        "{\"targets\": \"\", \"target_files\": [], \"client_configs\": \"\" }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  client_config_field_invalid_type,
        result);
}

TEST(RemoteConfigParser, TargetFilesAreParsed)
{
    std::string response = get_example_response();
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::success, result);

    EXPECT_EQ(2, gcr.get_target_files().size());

    auto target_files = gcr.get_target_files();

    EXPECT_EQ(
        "employee/DEBUG_DD/2.test1.config/config", target_files[0].get_path());
    EXPECT_EQ("UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=",
        target_files[0].get_raw());

    EXPECT_EQ(
        "datadog/2/DEBUG/luke.steensen/config", target_files[1].get_path());
    EXPECT_EQ("aGVsbG8gdmVjdG9yIQ==", target_files[1].get_raw());
}

TEST(RemoteConfigParser, TargetFilesWithoutPathAreInvalid)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{ \"raw\": "
        "\"aGVsbG8gdmVjdG9yIQ==\"} ], \"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");
    remote_config::get_configs_response gcr;
    auto result = remote_config::parse(invalid_response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  target_files_path_field_missing,
        result);
}

TEST(RemoteConfigParser, TargetFilesWithNonStringPathAreInvalid)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{\"path\": [], \"raw\": "
        "\"aGVsbG8gdmVjdG9yIQ==\"} ], \"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");
    remote_config::get_configs_response gcr;
    auto result = remote_config::parse(invalid_response, gcr);
    EXPECT_EQ(remote_config::remote_config_parser_result::
                  target_files_path_field_invalid_type,
        result);
}

TEST(RemoteConfigParser, TargetFilesWithoutRawAreInvalid)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{\"path\": \"datadog/2/DEBUG/luke.steensen/config\"} ], "
        "\"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(invalid_response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  target_files_raw_field_missing,
        result);
}

TEST(RemoteConfigParser, TargetFilesWithNonNonStringRawAreInvalid)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", \"target_files\": [{\"path\": "
        "\"employee/DEBUG_DD/2.test1.config/config\", \"raw\": "
        "\"UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=\"}, "
        "{\"path\": \"datadog/2/DEBUG/luke.steensen/config\", \"raw\": []} ], "
        "\"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");
    remote_config::get_configs_response gcr;
    auto result = remote_config::parse(invalid_response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  target_files_raw_field_invalid_type,
        result);
}

TEST(RemoteConfigParser, TargetFilesMustBeObjects)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", \"target_files\": [ "
        "\"invalid\", "
        "{\"path\": \"datadog/2/DEBUG/luke.steensen/config\", \"raw\": "
        "\"aGVsbG8gdmVjdG9yIQ==\"} ], \"client_configs\": "
        "[\"datadog/2/DEBUG/luke.steensen/config\", "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(invalid_response, gcr);

    EXPECT_EQ(
        remote_config::remote_config_parser_result::target_files_object_invalid,
        result);
}

TEST(RemoteConfigParser, ClientConfigsAreParsed)
{
    std::string response = get_example_response();
    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::success, result);

    EXPECT_EQ(2, gcr.get_client_configs().size());

    auto client_configs = gcr.get_client_configs();

    EXPECT_EQ("datadog/2/DEBUG/luke.steensen/config", client_configs[0]);
    EXPECT_EQ("employee/DEBUG_DD/2.test1.config/config", client_configs[1]);
}

TEST(RemoteConfigParser, ClientConfigsMustBeStrings)
{
    std::string invalid_response(
        "{\"roots\": [], \"targets\": \"b2s=\", "
        "\"target_files\": [], \"client_configs\": "
        "[[\"invalid\"], "
        "\"employee/DEBUG_DD/2.test1.config/config\"] }");

    remote_config::get_configs_response gcr;

    auto result = remote_config::parse(invalid_response, gcr);

    EXPECT_EQ(remote_config::remote_config_parser_result::
                  client_config_field_invalid_entry,
        result);
}

} // namespace dds
