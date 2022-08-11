// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client.hpp"
#include "protocol/tuf/parser.hpp"
#include "protocol/tuf/serializer.hpp"
#include <iostream>

namespace dds::remote_config {

protocol::get_configs_request client::generate_request()
{
    //@Todo when processing response
    std::vector<remote_config::protocol::config_state> config_states;

    protocol::client_tracer ct(std::move(this->_runtime_id),
        std::move(this->_tracer_version), std::move(this->_service),
        std::move(this->_env), std::move(this->_app_version));

    //@Todo generate situations with errors on tests
    protocol::client_state cs(0, std::move(config_states),
        this->_last_poll_error.size() > 0, std::move(this->_last_poll_error),
        std::move(this->_opaque_backend_state));
    dds::remote_config::protocol::client protocol_client(
        std::move(this->_id), std::move(this->_products), ct, cs);

    protocol::get_configs_request request(
        protocol_client, std::vector<protocol::cached_target_files>());

    return request;
};

protocol::remote_config_result client::process_response(
    protocol::get_configs_response response)
{
    std::map<std::string, remote_config::protocol::path> paths_on_targets =
        response.get_targets()->get_paths();
    for (std::string path : response.get_client_configs()) {
        auto path_itr = paths_on_targets.find(path);

        if (path_itr == paths_on_targets.end()) {
            this->_last_poll_error = "missing config " + path + " in targets";
            return protocol::remote_config_result::error;
        }
    }

    return protocol::remote_config_result::success;
}

protocol::remote_config_result client::poll()
{
    if (!this->_api) {
        return protocol::remote_config_result::error;
    }

    auto request = generate_request();

    std::string serialized_request;
    remote_config::protocol::serialize(request, serialized_request);

    std::string response_body;
    protocol::remote_config_result result =
        this->_api->get_configs(serialized_request, response_body);
    if (result == protocol::remote_config_result::error) {
        return protocol::remote_config_result::error;
    }

    protocol::get_configs_response response;
    auto parsing_result = protocol::parse(response_body, response);
    if (parsing_result != protocol::remote_config_parser_result::success) {
        return protocol::remote_config_result::error;
    }

    this->_last_poll_error = "";
    result = process_response(response);

    return result;
}

} // namespace dds::remote_config
