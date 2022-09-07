// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "base64.h"
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

class cout_listener : public remote_config::product_listener {
public:
    void config_to_cout(remote_config::config config)
    {
        std::cout << "path: " << config.get_path() << std::endl;
        std::cout << "version: " << config.get_version() << std::endl;
        std::cout << "id: " << config.get_id() << std::endl;
        std::cout << "length: " << config.get_length() << std::endl;
        std::cout << "contents: " << config.get_contents() << std::endl;
        std::cout << "product: " << config.get_product() << std::endl;
        for (auto hash : config.get_hashes()) {
            std::cout << "hash: " << hash.first << " - " << hash.second
                      << std::endl;
        }
        std::cout << "---------------" << std::endl;
    }

    void on_update(const std::vector<remote_config::config> &configs) override
    {
        std::cout << std::endl
                  << "Product update " << std::endl
                  << "==============" << std::endl;

        for (auto c : configs) { config_to_cout(c); }
    };
    void on_unapply(const std::vector<remote_config::config> &configs) override
    {
        std::cout << std::endl
                  << "Product removing configs " << std::endl
                  << "==============" << std::endl;

        for (auto c : configs) { config_to_cout(c); }
    };
};

namespace mock {

// The simple custom action
ACTION_P(set_response_body, response) { arg1.assign(response); }

class api : public remote_config::http_api {
public:
    MOCK_METHOD((std::pair<remote_config::protocol::remote_config_result,
                    std::optional<std::string>>),
        get_configs, (std::string && request), (const override));
};

class listener_mock : public remote_config::product_listener {
public:
    MOCK_METHOD(void, on_update,
        (const std::vector<remote_config::config> &configs), (override));
    MOCK_METHOD(void, on_unapply,
        (const std::vector<remote_config::config> &configs), (override));
};
} // namespace mock

namespace test_helpers {
std::string sha256_from_path(std::string path) { return path + "_sha256"; }

std::string raw_from_path(std::string path) { return path + "_raw"; }

// Just a deterministic way of asserting this and avoid hadcoding much
int version_from_path(std::string path) { return path.length() + 55; }

int length_from_path(std::string path) { return path.length(); }
} // namespace test_helpers

class RemoteConfigClient : public ::testing::Test {
public:
    std::string id;
    std::string runtime_id;
    std::string tracer_version;
    std::string service;
    std::string env;
    std::string app_version;
    std::string backend_client_state;
    int target_version;
    std::string features;
    std::string asm_dd;
    std::string apm_sampling;
    std::vector<std::string> products_str;
    std::vector<remote_config::product> _products;
    std::string first_product_product;
    std::string first_product_id;
    std::string second_product_product;
    std::string second_product_id;
    std::string first_path;
    std::string second_path;
    std::vector<std::string> paths;

    void SetUp()
    {
        // Since most values are moved to the classes, they need to be generated
        // again on each set up
        id = "some id";
        runtime_id = "some runtime id";
        tracer_version = "some tracer version";
        service = "some service";
        env = "some env";
        app_version = "some app version";
        backend_client_state = "some backend state here";
        target_version = 123;
        features = "FEATURES";
        asm_dd = "ASM_DD";
        apm_sampling = "APM_SAMPLING";
        products_str = {asm_dd, features};

        first_product_product = features;
        first_product_id = "luke.steensen";
        second_product_product = features;
        second_product_id = "2.test1.config";
        first_path = "datadog/2/" + first_product_product + "/" +
                     first_product_id + "/config";
        second_path = "employee/" + second_product_product + "/" +
                      second_product_id + "/config";
        paths = {first_path, second_path};
        generate_products();
    }

    void generate_products()
    {
        for (const std::string &p_str : products_str) {
            std::string product_name(p_str);
            remote_config::product _p(std::move(product_name), {});
            _products.push_back(_p);
        }
    }

    remote_config::protocol::client generate_client(bool generate_state)
    {
        auto runtime_id_cpy = runtime_id;
        auto tracer_version_cpy = tracer_version;
        auto service_cpy = service;
        auto env_cpy = env;
        auto app_version_cpy = app_version;
        remote_config::protocol::client_tracer client_tracer(
            std::move(runtime_id_cpy), std::move(tracer_version_cpy),
            std::move(service_cpy), std::move(env_cpy),
            std::move(app_version_cpy));

        std::vector<remote_config::protocol::config_state> config_states;
        int _target_version;
        std::string _backend_client_state;
        if (generate_state) {
            // All these states are extracted from the harcoded request/response
            std::string product00(first_product_product);
            std::string product00_id(first_product_id);
            remote_config::protocol::config_state cs00(std::move(product00_id),
                test_helpers::version_from_path(first_path),
                std::move(product00));
            std::string product01(second_product_product);
            std::string product01_id(second_product_id);
            remote_config::protocol::config_state cs01(std::move(product01_id),
                test_helpers::version_from_path(second_path),
                std::move(product01));

            config_states.push_back(cs00);
            config_states.push_back(cs01);
            _target_version = target_version;
            // This field is extracted from the harcoded resposne
            _backend_client_state = backend_client_state;
        } else {
            _target_version = 0; // Default target version
            _backend_client_state = "";
        }
        remote_config::protocol::client_state client_state(_target_version,
            std::move(config_states), false, "",
            std::move(_backend_client_state));

        auto products_str_cpy = products_str;
        auto id_cpy = id;
        remote_config::protocol::client client(std::move(id_cpy),
            std::move(products_str_cpy), std::move(client_tracer),
            std::move(client_state));

        return client;
    }

    std::string generate_targets(
        std::vector<std::string> paths, std::string opaque_backend_state)
    {
        std::string targets_str;
        for (int i = 0; i < paths.size(); i++) {
            std::string path = paths[i];
            std::string sha256 = test_helpers::sha256_from_path(path);
            targets_str.append(
                ("\"" + path + "\": {\"custom\": {\"v\": " +
                    std::to_string(test_helpers::version_from_path(path)) +
                    " }, \"hashes\": {\"sha256\": \"" + sha256 +
                    "\"}, \"length\": " +
                    std::to_string(test_helpers::length_from_path(paths[i])) +
                    " }"));
            if (i + 1 < paths.size()) {
                targets_str.append(",");
            }
        }

        std::string targets_json =
            ("{\"signatures\": [], \"signed\": {\"_type\": \"targets\", "
             "\"custom\": {\"opaque_backend_state\": \"" +
                opaque_backend_state +
                "\"}, "
                "\"expires\": \"2022-11-04T13:31:59Z\", \"spec_version\": "
                "\"1.0.0\", \"targets\": {" +
                targets_str +
                "}, \"version\": " + std::to_string(target_version) + " } }");

        return base64_encode(targets_json);
    }

    std::string generate_example_response(
        std::vector<std::string> client_configs,
        std::vector<std::string> target_files,
        std::vector<std::string> target_paths)
    {
        std::string client_configs_str = "";
        std::string target_files_str = "";
        for (int i = 0; i < client_configs.size(); i++) {
            client_configs_str.append("\"" + client_configs[i] + "\"");
            if (i + 1 < client_configs.size()) {
                client_configs_str.append(", ");
            }
        }
        for (int i = 0; i < target_files.size(); i++) {
            target_files_str.append(
                "{\"path\": \"" + target_files[i] + "\", \"raw\": \"" +
                test_helpers::raw_from_path(target_files[i]) + "\"}");
            if (i + 1 < target_files.size()) {
                target_files_str.append(",");
            }
        }
        return ("{\"roots\": [], \"targets\": \"" +
                generate_targets(target_paths, backend_client_state) +
                "\", \"target_files\": [" + target_files_str +
                "], "
                "\"client_configs\": [" +
                client_configs_str +
                "] "
                "}");
    }

    std::string generate_example_response(std::vector<std::string> paths)
    {
        return generate_example_response(paths, paths, paths);
    }

    remote_config::protocol::get_configs_request generate_request(
        bool generate_state, bool generate_cache)
    {
        dds::remote_config::protocol::client protocol_client =
            generate_client(generate_state);
        std::vector<remote_config::protocol::cached_target_files> files;
        if (generate_cache) {
            // First cached file
            remote_config::protocol::cached_target_files_hash hash01(
                "sha256", test_helpers::sha256_from_path(paths[0]));
            std::string path01 = paths[0];
            remote_config::protocol::cached_target_files file01(
                std::move(path01), test_helpers::length_from_path(path01),
                {hash01});
            files.push_back(file01);

            // Second cached file
            remote_config::protocol::cached_target_files_hash hash02(
                "sha256", test_helpers::sha256_from_path(paths[1]));
            std::string path02 = paths[1];
            remote_config::protocol::cached_target_files file02(
                std::move(path02), test_helpers::length_from_path(path02),
                {hash02});
            files.push_back(file02);
        }
        remote_config::protocol::get_configs_request request(
            std::move(protocol_client), std::move(files));

        return request;
    }

    std::string generate_request_serialized(
        bool generate_state, bool generate_cache)
    {
        std::optional<std::string> request_serialized;

        request_serialized = remote_config::protocol::serialize(
            generate_request(generate_state, generate_cache));

        return request_serialized.value();
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

    std::pair<remote_config::protocol::remote_config_result,
        std::optional<std::string>>
    get_config_response(
        remote_config::protocol::remote_config_result result, std::string body)
    {
        std::pair<remote_config::protocol::remote_config_result,
            std::optional<std::string>>
            pair(result, body);

        return pair;
    }
};

TEST_F(RemoteConfigClient, ItReturnsErrorIfApiReturnsError)
{
    mock::api *const mock_api = new mock::api;
    EXPECT_CALL(*mock_api, get_configs)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::error, "")));

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
    delete mock_api;
}

TEST_F(RemoteConfigClient, ItCallsToApiOnPoll)
{
    mock::api *const mock_api = new mock::api;
    // First poll dont have state
    EXPECT_CALL(
        *mock_api, get_configs(generate_request_serialized(false, false)))
        .Times(AtLeast(1))
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths))));

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
    delete mock_api;
}

TEST_F(RemoteConfigClient, ItReturnErrorWhenApiNotProvided)
{
    dds::remote_config::client api_client(nullptr, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST_F(RemoteConfigClient, ItReturnErrorWhenResponseIsInvalidJson)
{
    mock::api mock_api;
    EXPECT_CALL(mock_api, get_configs)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            "invalid json here")));

    dds::remote_config::client api_client(&mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::error, result);
}

TEST_F(RemoteConfigClient,
    ItReturnErrorAndSaveLastErrorWhenClientConfigPathNotInTargetPaths)
{
    std::string response = generate_example_response(paths, paths, {});

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                response))));

    dds::remote_config::client api_client(&mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

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
        "missing config " + paths[0] +
            " in "
            "targets"));
}

TEST_F(RemoteConfigClient,
    ItReturnErrorAndSaveLastErrorWhenClientConfigPathNotInTargetFiles)
{
    std::string response = generate_example_response(paths, {}, paths);

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                response))));

    dds::remote_config::client api_client(&mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

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
        "missing config " + paths[0] +
            " in "
            "target files and in cache files"));
}

TEST(ClientConfig, ItGetGeneratedFromString)
{
    std::string apm_sampling = "apm_sampling";
    auto cp = remote_config::config_path_from_path(
        "datadog/2/LIVE_DEBUGGING/9e413cda-647b-335b-adcd-7ce453fc2284/config");
    EXPECT_TRUE(cp);
    EXPECT_EQ("LIVE_DEBUGGING", cp->get_product());
    EXPECT_EQ("9e413cda-647b-335b-adcd-7ce453fc2284", cp->get_id());

    cp = remote_config::config_path_from_path(
        "employee/DEBUG_DD/2.test1.config/config");
    EXPECT_TRUE(cp);
    EXPECT_EQ("DEBUG_DD", cp->get_product());
    EXPECT_EQ("2.test1.config", cp->get_id());

    cp = remote_config::config_path_from_path(
        "datadog/55/APM_SAMPLING/dynamic_rates/config");
    EXPECT_TRUE(cp);
    EXPECT_EQ(apm_sampling, cp->get_product());
    EXPECT_EQ("dynamic_rates", cp->get_id());
}

TEST(ClientConfig, ItDoesNotGetGeneratedFromStringIfNotValidMatch)
{
    remote_config::protocol::remote_config_result result;

    auto cp = remote_config::config_path_from_path("");
    EXPECT_FALSE(cp);

    cp = remote_config::config_path_from_path("invalid");
    EXPECT_FALSE(cp);

    cp = remote_config::config_path_from_path("datadog/55/APM_SAMPLING/config");
    EXPECT_FALSE(cp);

    cp =
        remote_config::config_path_from_path("datadog/55/APM_SAMPLING//config");
    EXPECT_FALSE(cp);

    cp = remote_config::config_path_from_path(
        "datadog/aa/APM_SAMPLING/dynamic_rates/config");
    EXPECT_FALSE(cp);

    cp = remote_config::config_path_from_path(
        "something/APM_SAMPLING/dynamic_rates/config");
    EXPECT_FALSE(cp);
}

TEST_F(RemoteConfigClient, ItReturnsErrorWhenClientConfigPathCantBeParsed)
{
    std::string invalid_path = "invalid/path/dynamic_rates/config";
    std::string response = generate_example_response({invalid_path});

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                response))));

    dds::remote_config::client api_client(&mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    // Validate first request does not contain any error
    auto poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(request_sent, false, ""));

    // Validate second request contains error
    poll_result = api_client.poll();
    EXPECT_EQ(
        remote_config::protocol::remote_config_result::error, poll_result);
    EXPECT_TRUE(validate_request_has_error(
        request_sent, true, "error parsing path " + invalid_path));
}

TEST_F(RemoteConfigClient, ItReturnsErrorIfProductOnPathNotRequested)
{
    std::string path_of_no_requested_product =
        "datadog/2/APM_SAMPLING/dynamic_rates/config";
    std::string response =
        generate_example_response({path_of_no_requested_product});

    mock::api mock_api;
    std::string request_sent;
    EXPECT_CALL(mock_api, get_configs)
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                response))));

    std::vector<remote_config::product_listener *> listeners;
    dds::remote_config::client api_client(&mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), {});

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
        "received config " + path_of_no_requested_product +
            " for a "
            "product that was not requested"));
}

TEST_F(RemoteConfigClient, ItGeneratesClientStateAndCacheFromResponse)
{
    mock::api *const mock_api = new mock::api;

    // First call should not contain state neither cache
    EXPECT_CALL(
        *mock_api, get_configs(generate_request_serialized(false, false)))
        .Times(1)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths))));

    // Second call. This should contain state and cache from previous
    EXPECT_CALL(*mock_api, get_configs(generate_request_serialized(true, true)))
        .Times(1)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths))));

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST_F(RemoteConfigClient, WhenANewConfigIsAddedItCallsOnUpdateOnPoll)
{
    mock::api *const mock_api = new mock::api;

    std::string response = generate_example_response({first_path});

    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(1)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success, response)));

    std::string content = test_helpers::raw_from_path(first_path);
    std::map<std::string, std::string> hashes = {
        std::pair<std::string, std::string>(
            "sha256", test_helpers::sha256_from_path(first_path))};
    std::string first_product_product_cpy = first_product_product;
    std::string first_product_id_cpy = first_product_id;
    std::string first_path_cpy = first_path;
    remote_config::config expected_config(std::move(first_product_product_cpy),
        std::move(first_product_id_cpy), std::move(content), std::move(hashes),
        test_helpers::version_from_path(first_path), std::move(first_path_cpy),
        test_helpers::length_from_path(first_path));

    // Product on response
    std::vector<remote_config::config> no_updates;
    mock::listener_mock listener01;
    EXPECT_CALL(listener01,
        on_update(
            std::move(std::vector<remote_config::config>{expected_config})))
        .Times(1);
    EXPECT_CALL(listener01, on_unapply(std::vector<remote_config::config>{}))
        .Times(1);
    mock::listener_mock listener02;
    EXPECT_CALL(listener02,
        on_update(
            std::move(std::vector<remote_config::config>{expected_config})))
        .Times(1);
    EXPECT_CALL(listener02, on_unapply(std::vector<remote_config::config>{}))
        .Times(1);
    remote_config::product product(
        std::move(first_product_product), {&listener01, &listener02});

    // Product on response
    mock::listener_mock listener_called_no_configs01;
    EXPECT_CALL(listener_called_no_configs01,
        on_update(std::vector<remote_config::config>{}))
        .Times(1);
    EXPECT_CALL(listener_called_no_configs01,
        on_unapply(std::vector<remote_config::config>{}))
        .Times(1);
    mock::listener_mock listener_called_no_configs02;
    EXPECT_CALL(listener_called_no_configs02,
        on_update(std::vector<remote_config::config>{}))
        .Times(1);
    EXPECT_CALL(listener_called_no_configs02,
        on_unapply(std::vector<remote_config::config>{}))
        .Times(1);
    std::string product_str_not_in_response = "NOT_IN_RESPONSE";
    remote_config::product product_not_in_response(
        std::move(product_str_not_in_response),
        {&listener_called_no_configs01, &listener_called_no_configs02});

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version),
        {product, product_not_in_response});

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST_F(RemoteConfigClient, WhenAConfigDissapearOnFollowingPollsItCallsToUnApply)
{
    mock::api *const mock_api = new mock::api;

    std::string response01 = generate_example_response({first_path});

    std::string response02 = generate_example_response({second_path});

    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(2)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            response01)))
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            response02)));

    std::string content01 = test_helpers::raw_from_path(first_path);
    std::map<std::string, std::string> hashes01 = {
        std::pair<std::string, std::string>(
            "sha256", test_helpers::sha256_from_path(first_path))};
    std::string first_product_product_cpy = first_product_product;
    std::string first_product_id_cpy = first_product_id;
    std::string first_path_cpy = first_path;
    remote_config::config expected_config01(
        std::move(first_product_product_cpy), std::move(first_product_id_cpy),
        std::move(content01), std::move(hashes01),
        test_helpers::version_from_path(first_path), std::move(first_path_cpy),
        test_helpers::length_from_path(first_path));
    std::vector<remote_config::config> no_updates;

    std::string content02 = test_helpers::raw_from_path(second_path);
    std::map<std::string, std::string> hashes02 = {
        std::pair<std::string, std::string>(
            "sha256", test_helpers::sha256_from_path(second_path))};
    std::string second_product_product_cpy = second_product_product;
    std::string second_product_id_cpy = second_product_id;
    std::string second_path_cpy = second_path;
    remote_config::config expected_config02(
        std::move(second_product_product_cpy), std::move(second_product_id_cpy),
        std::move(content02), std::move(hashes02),
        test_helpers::version_from_path(second_path),
        std::move(second_path_cpy),
        test_helpers::length_from_path(second_path));

    // Product on response
    mock::listener_mock listener01;
    // Second poll expectations
    EXPECT_CALL(listener01,
        on_update(std::vector<remote_config::config>{expected_config02}))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01,
        on_unapply(std::vector<remote_config::config>{expected_config01}))
        .Times(1)
        .RetiresOnSaturation();
    // First poll expectations
    EXPECT_CALL(listener01,
        on_update(std::vector<remote_config::config>{expected_config01}))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(std::vector<remote_config::config>{}))
        .Times(1)
        .RetiresOnSaturation();
    remote_config::product product(
        std::move(first_product_product), {&listener01});

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), {product});

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST_F(
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
        "\"client_configs\": "
        "[\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
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
        "\"client_configs\": "
        "[\"datadog/2/APM_SAMPLING/dynamic_rates/config\"] "
        "}");

    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(2)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            response01)))
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            response02)));

    std::string product_str = "APM_SAMPLING";
    std::string product_str_01 = product_str;
    std::string product_str_02 = product_str;
    std::string id_product = "dynamic_rates";
    std::string id_product_01 = id_product;
    std::string id_product_02 = id_product;
    std::string path = "datadog/2/APM_SAMPLING/dynamic_rates/config";
    std::string path_01 = path;
    std::string path_02 = path;
    std::string content =
        "UmVtb3RlIGNvbmZpZ3VyYXRpb24gaXMgc3VwZXIgc3VwZXIgY29vbAo=";
    std::string content_01 = content;
    std::string content_02 = content;
    std::map<std::string, std::string> hashes_01 = {std::pair<std::string,
        std::string>("sha256",
        "07465cece47e4542abc0da040d9ebb42ec97224920d6870651dc3316528609d5")};
    remote_config::config expected_config(std::move(product_str_01),
        std::move(id_product_01), std::move(content_01), std::move(hashes_01),
        36740, std::move(path_01), 66399);
    std::vector<remote_config::config> expected_configs_first_call = {
        expected_config};
    std::vector<remote_config::config> no_updates;

    std::map<std::string, std::string> hashes_02 = {
        std::pair<std::string, std::string>("sha256", "another_hash_here")};
    remote_config::config expected_config_02(std::move(product_str_02),
        std::move(id_product_02), std::move(content_02), std::move(hashes_02),
        36740, std::move(path_02), 66399);
    std::vector<remote_config::config> expected_configs_second_call = {
        expected_config_02};

    // Product on response
    mock::listener_mock listener01;
    // Second poll expectations
    EXPECT_CALL(listener01, on_update(std::move(expected_configs_second_call)))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(std::vector<remote_config::config>{}))
        .Times(1)
        .RetiresOnSaturation();
    // First poll expectations
    EXPECT_CALL(listener01, on_update(std::move(expected_configs_first_call)))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(listener01, on_unapply(std::vector<remote_config::config>{}))
        .Times(1)
        .RetiresOnSaturation();
    remote_config::product product(std::move(apm_sampling), {&listener01});

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), {product});

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST_F(RemoteConfigClient, FilesThatAreInCacheAreUsedWhenNotInTargetFiles)
{
    mock::api *const mock_api = new mock::api;

    // First call should not contain state neither cache
    EXPECT_CALL(
        *mock_api, get_configs(generate_request_serialized(false, false)))
        .Times(1)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths))))
        .RetiresOnSaturation();

    // Second call. Since this call has cache, response comes without
    // target_files
    // Third call. Cache and state should be keeped even though
    // target_files came empty on second
    EXPECT_CALL(*mock_api, get_configs(generate_request_serialized(true, true)))
        .Times(2)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths, {}, paths))))
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths, {}, paths))));

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

    auto result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    result = api_client.poll();
    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);

    delete mock_api;
}

TEST_F(RemoteConfigClient, NotTrackedFilesAreDeletedFromCache)
{
    mock::api *const mock_api = new mock::api;

    std::string request_sent;
    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(3)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response(paths))))
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            generate_example_response({}))))
        .WillOnce(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                generate_example_response({})))));

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

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

TEST_F(RemoteConfigClient, TestHashIsDifferentFromTheCache)
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
        "\"employee/FEATURES/2.test1.config/config\", \"raw\": \"some_raw=\"}"
        "], \"client_configs\": [\"employee/FEATURES/2.test1.config/config\"]"
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
    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(3)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            first_response)))
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                second_response))))
        .RetiresOnSaturation();

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

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

TEST_F(RemoteConfigClient, TestWhenFileGetsFromCacheItsCachedLenUsed)
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
        "\"employee/FEATURES/2.test1.config/config\", \"raw\": \"some_raw=\"}"
        "], \"client_configs\": [\"employee/FEATURES/2.test1.config/config\"]"
        "}";

    // This response has a cached file with different len and version, it
    // should not be used
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
    EXPECT_CALL(*mock_api, get_configs(_))
        .Times(3)
        .WillOnce(Return(get_config_response(
            remote_config::protocol::remote_config_result::success,
            first_response)))
        .WillRepeatedly(DoAll(testing::SaveArg<0>(&request_sent),
            Return(get_config_response(
                remote_config::protocol::remote_config_result::success,
                second_response))))
        .RetiresOnSaturation();

    dds::remote_config::client api_client(mock_api, std::move(id),
        std::move(runtime_id), std::move(tracer_version), std::move(service),
        std::move(env), std::move(app_version), _products);

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
TEST_F(RemoteConfigClient, TestAgainstDocker)
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
