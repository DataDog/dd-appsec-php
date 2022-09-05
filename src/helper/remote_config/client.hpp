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
    config_path(std::string &&id, std::string &&product)
        : id_(std::move(id)), product_(std::move(product)){};
    std::string get_id() { return id_; };
    std::string get_product() { return product_; };

private:
    std::string product_;
    std::string id_;
};

std::optional<config_path> config_path_from_path(const std::string &path);

class client {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    client(http_api *arg_api, std::string &id, std::string &runtime_id,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        std::string &tracer_version, std::string &service, std::string &env,
        std::string &app_version, std::vector<product> &products)
        : api_(arg_api), id_(id), runtime_id_(runtime_id),
          tracer_version_(tracer_version), service_(service), env_(env),
          app_version_(app_version), targets_version_(0)
    {
        for (auto &product : products) {
            products_.insert(std::pair<std::string, remote_config::product>(
                product.get_name(), product));
        }
    };

    protocol::remote_config_result poll();

private:
    protocol::get_configs_request generate_request();
    protocol::remote_config_result process_response(
        protocol::get_configs_response &response);

    http_api *api_;
    std::string id_;
    std::string runtime_id_;
    std::string tracer_version_;
    std::string service_;
    std::string env_;
    std::string app_version_;
    std::string last_poll_error_;
    std::string opaque_backend_state_;
    int targets_version_;
    std::map<std::string, product> products_;
};

} // namespace dds::remote_config
