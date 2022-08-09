// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

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
class api : public remote_config::http_api {
public:
    MOCK_METHOD(remote_config::protocol::remote_config_result, get_configs,
        (remote_config::protocol::get_configs_request request), (override));
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
std::vector<remote_config::protocol::product> products2 = {
    remote_config::protocol::product::asm_dd,
    remote_config::protocol::product::features};

remote_config::protocol::client get_client_two()
{
    remote_config::protocol::client_tracer client_tracer(runtime_id.c_str(),
        tracer_version.c_str(), service.c_str(), env.c_str(),
        app_version.c_str());

    std::vector<remote_config::protocol::config_state> config_states;
    remote_config::protocol::client_state client_state(target_version,
        std::move(config_states), false, "", backend_client_state.c_str());

    std::string client_id(id);
    remote_config::protocol::client client(
        std::move(client_id), std::move(products), client_tracer, client_state);

    return client;
}

TEST(RemoteConfigClient, ItCallsToApiOnPoll)
{
    dds::remote_config::protocol::client protocol_client = get_client_two();
    std::vector<remote_config::protocol::cached_target_files> files;
    remote_config::protocol::get_configs_request expected_request(
        protocol_client, std::move(files));

    mock::api mock_api;
    EXPECT_CALL(mock_api, get_configs(expected_request))
        .Times(1)
        .WillRepeatedly(
            Return(remote_config::protocol::remote_config_result::success));

    dds::remote_config::client api_client(&mock_api, id.c_str(),
        runtime_id.c_str(), tracer_version.c_str(), service.c_str(),
        env.c_str(), app_version.c_str(), std::move(products2));

    auto result = api_client.poll();

    EXPECT_EQ(remote_config::protocol::remote_config_result::success, result);
}

} // namespace dds
