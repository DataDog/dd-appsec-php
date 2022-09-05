// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

#include "http_api.hpp"
#include "product.hpp"
#include "protocol/client.hpp"
#include "protocol/tuf/get_configs_response.hpp"

namespace dds::remote_config {

struct config_path {
public:
    config_path() = default;
    std::string get_id() { return _id; };
    std::string get_product() { return _product; };
    void set_id(std::string &&id) { _id = std::move(id); };
    void set_product(std::string &&product) { _product = std::move(product); };

private:
    std::string _product;
    std::string _id;
};

protocol::remote_config_result config_path_from_path(
    const std::string &path, config_path &cp);

class client {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    client(http_api *arg_api, std::string &id, std::string &runtime_id,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        std::string &tracer_version, std::string &service, std::string &env,
        std::string &app_version, std::vector<product> &products)
        : _api(arg_api), _id(id), _runtime_id(runtime_id),
          _tracer_version(tracer_version), _service(service), _env(env),
          _app_version(app_version), _targets_version(0)
    {
        for (product &p : products) {
            _products.insert(std::pair<std::string, remote_config::product>(
                p.get_name(), p));
        }
    };

    protocol::remote_config_result poll();

private:
    protocol::get_configs_request generate_request();
    protocol::remote_config_result process_response(
        protocol::get_configs_response response);

    http_api *_api;
    std::string _id;
    std::string _runtime_id;
    std::string _tracer_version;
    std::string _service;
    std::string _env;
    std::string _app_version;
    std::string _last_poll_error;
    std::string _opaque_backend_state;
    int _targets_version;
    std::map<std::string, remote_config::product> _products;
};

} // namespace dds::remote_config
