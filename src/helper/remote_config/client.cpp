// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client.hpp"
#include "protocol/tuf/serializer.hpp"
#include <iostream>

namespace dds::remote_config {

protocol::remote_config_result client::poll()
{
    //@Todo when processing response
    std::vector<remote_config::protocol::config_state> config_states;
    protocol::client_tracer ct(std::move(this->_runtime_id),
        std::move(this->_tracer_version), std::move(this->_service),
        std::move(this->_env), std::move(this->_app_version));
    protocol::client_state cs(0, std::move(config_states),
        this->_last_poll_error.size() > 0, std::move(this->_last_poll_error),
        std::move(this->_opaque_backend_state));
    dds::remote_config::protocol::client protocol_client(
        std::move(this->_id), std::move(this->_products), ct, cs);

    protocol::get_configs_request request(
        protocol_client, std::vector<protocol::cached_target_files>());

    this->_api->get_configs(request);
    return protocol::remote_config_result::success;
}

} // namespace dds::remote_config
