// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client.hpp"
#include "protocol/tuf/parser.hpp"
#include "protocol/tuf/serializer.hpp"
#include <regex>

namespace dds::remote_config {

protocol::remote_config_result config_path_from_path(
    std::string path, config_path &cp)
{
    std::regex regex("^(datadog/\\d+|employee)/([^/]+)/([^/]+)/[^/]+$");

    std::smatch base_match;
    if (!std::regex_match(path, base_match, regex) || base_match.size() < 4) {
        return protocol::remote_config_result::error;
    }

    cp.set_product(base_match[2].str());
    cp.set_id(base_match[3].str());

    return protocol::remote_config_result::success;
}

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
    std::vector<protocol::product_e> products(this->_products);
    dds::remote_config::protocol::client protocol_client(
        std::move(this->_id), std::move(products), ct, cs);

    protocol::get_configs_request request(
        protocol_client, std::vector<protocol::cached_target_files>());

    return request;
};

protocol::remote_config_result product_from_string(
    const std::string &product_str, protocol::product_e &parsed)
{
    if (product_str == "LIVE_DEBUGGING") {
        parsed = protocol::product_e::live_debugging;
        return protocol::remote_config_result::success;
    } else if (product_str == "ASM_DD") {
        parsed = protocol::product_e::asm_dd;
        return protocol::remote_config_result::success;
    } else if (product_str == "FEATURES") {
        parsed = protocol::product_e::features;
        return protocol::remote_config_result::success;
    }

    return protocol::remote_config_result::error;
}

protocol::remote_config_result client::process_response(
    protocol::get_configs_response response)
{
    std::map<std::string, remote_config::protocol::path> paths_on_targets =
        response.get_targets()->get_paths();
    std::map<std::string, remote_config::protocol::target_file> target_files =
        response.get_target_files();
    for (std::string path : response.get_client_configs()) {
        config_path cp;
        if (config_path_from_path(path, cp) !=
            protocol::remote_config_result::success) {
            this->_last_poll_error = "error parsing path " + path;
            return protocol::remote_config_result::error;
        }

        // Is path on targets?
        auto path_itr = paths_on_targets.find(path);
        if (path_itr == paths_on_targets.end()) {
            // Not found
            this->_last_poll_error = "missing config " + path + " in targets";
            return protocol::remote_config_result::error;
        }

        // Is path on target_files?
        auto path_in_target_files = target_files.find(path);
        if (path_in_target_files == target_files.end()) {
            // Not found
            this->_last_poll_error =
                "missing config " + path + " in target files";
            return protocol::remote_config_result::error;
        }

        // Is product on the requested?
        protocol::product product_on_path;
        if (product_from_string(cp.get_product(), product_on_path) ==
                protocol::remote_config_result::error ||
            std::find(this->_products.begin(), this->_products.end(),
                product_on_path) == this->_products.end()) {
            // Not found
            this->_last_poll_error = "received config " + path +
                                     " for a product that was not requested";
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
