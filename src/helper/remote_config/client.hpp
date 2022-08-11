// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

#include "http_api.hpp"
#include "protocol/client.hpp"
#include "protocol/tuf/get_configs_response.hpp"

namespace dds::remote_config {

class client {
public:
    client(http_api *arg_api, std::string &&id, std::string &&runtime_id,
        std::string &&tracer_version, std::string &&service, std::string &&env,
        std::string &&app_version, std::vector<protocol::product> &&products)
        : _api(arg_api), _id(std::move(id)), _runtime_id(std::move(runtime_id)),
          _tracer_version(std::move(tracer_version)),
          _service(std::move(service)), _env(std::move(env)),
          _app_version(std::move(app_version)),
          _products(std::move(products)){};

    protocol::remote_config_result poll();

private:
    protocol::get_configs_request generate_request();
    protocol::remote_config_result process_response(protocol::get_configs_response response);

    http_api *_api;
    std::string _id;
    std::string _runtime_id;
    std::string _tracer_version;
    std::string _service;
    std::string _env;
    std::string _app_version;
    std::string _last_poll_error;
    std::string _opaque_backend_state;
    std::vector<protocol::product> _products;
};

} // namespace dds::remote_config
