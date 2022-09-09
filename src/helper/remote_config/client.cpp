// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client.hpp"
#include "product.hpp"
#include "protocol/tuf/parser.hpp"
#include "protocol/tuf/serializer.hpp"
#include <algorithm>
#include <map>
#include <regex>

namespace dds::remote_config {

std::optional<config_path> config_path_from_path(const std::string &path)
{
    std::regex regex("^(datadog/\\d+|employee)/([^/]+)/([^/]+)/[^/]+$");

    std::smatch base_match;
    if (!std::regex_match(path, base_match, regex) || base_match.size() < 4) {
        return std::nullopt;
    }

    config_path cp(base_match[3].str(), base_match[2].str());

    return cp;
}

[[nodiscard]] protocol::get_configs_request client::generate_request() const
{
    std::vector<protocol::config_state> config_states;
    std::vector<protocol::cached_target_files> files;

    for (const auto &[product_name, product] : products_) {
        // State
        auto configs_on_product = product.get_configs();
        for (auto &config : configs_on_product) {
            config_states.emplace_back(
                config.get_id(), config.get_version(), config.get_product());
        }

        // Cached files
        auto configs = product.get_configs();
        for (auto &config : configs) {
            std::vector<protocol::cached_target_files_hash> hashes;
            hashes.reserve(config.get_hashes().size());
            for (auto const &[algo, hash_sting] : config.get_hashes()) {
                auto algo_cpy = algo;
                auto hash_sting_cpy = hash_sting;
                hashes.emplace_back(
                    std::move(algo_cpy), std::move(hash_sting_cpy));
            }
            files.emplace_back(
                config.get_path(), config.get_length(), std::move(hashes));
        }
    }

    auto runtime_id_cpy = runtime_id_;
    auto tracer_version_cpy = tracer_version_;
    auto service_cpy = service_;
    auto env_cpy = env_;
    auto app_version_cpy = app_version_;
    protocol::client_tracer ct(std::move(runtime_id_cpy),
        std::move(tracer_version_cpy), std::move(service_cpy),
        std::move(env_cpy), std::move(app_version_cpy));

    auto last_poll_error_cpy = last_poll_error_;
    auto opaque_backend_state_cpy = opaque_backend_state_;
    protocol::client_state cs(targets_version_, std::move(config_states),
        !last_poll_error_.empty(), std::move(last_poll_error_cpy),
        std::move(opaque_backend_state_cpy));
    std::vector<std::string> products_str;
    products_str.reserve(products_.size());
    for (const auto &[product_name, product] : products_) {
        products_str.push_back(product_name);
    }
    std::string id_cpy = id_;
    protocol::client protocol_client(std::move(id_cpy), std::move(products_str),
        std::move(ct), std::move(cs));

    protocol::get_configs_request request(
        std::move(protocol_client), std::move(files));

    return request;
};

protocol::remote_config_result client::process_response(
    protocol::get_configs_response &&response)
{
    const std::map<std::string, protocol::path> paths_on_targets =
        response.get_targets().get_paths();
    const std::map<std::string, protocol::target_file> target_files =
        response.get_target_files();
    std::map<std::string, std::vector<config>> configs;
    for (const std::string &path : response.get_client_configs()) {
        std::optional<config_path> cp;
        cp = config_path_from_path(path);
        if (!cp) {
            last_poll_error_ = "error parsing path " + path;
            return protocol::remote_config_result::error;
        }

        // Is path on targets?
        auto path_itr = paths_on_targets.find(path);
        int length;
        if (path_itr == paths_on_targets.end()) {
            // Not found
            last_poll_error_ = "missing config " + path + " in targets";
            return protocol::remote_config_result::error;
        }
        length = path_itr->second.get_length();
        std::map<std::string, std::string> hashes =
            path_itr->second.get_hashes();
        int custom_v = path_itr->second.get_custom_v();

        // Is product on the requested ones?
        auto product = products_.find(cp->get_product());
        if (product == products_.end()) {
            // Not found
            last_poll_error_ = "received config " + path +
                               " for a product that was not requested";
            return protocol::remote_config_result::error;
        }

        // Is path on target_files?
        auto path_in_target_files = target_files.find(path);
        std::string raw;
        if (path_in_target_files == target_files.end()) {
            // Check if file in cache
            auto configs_on_product = product->second.get_configs();
            auto config_itr = std::find_if(configs_on_product.begin(),
                configs_on_product.end(), [&path, &hashes](config &c) {
                    return c.get_path() == path && c.get_hashes() == hashes;
                });

            if (config_itr == configs_on_product.end()) {
                // Not found
                last_poll_error_ = "missing config " + path +
                                   " in target files and in cache files";
                return protocol::remote_config_result::error;
            }

            raw = config_itr->get_contents();
            length = config_itr->get_length();
            custom_v = config_itr->get_version();
        } else {
            raw = path_in_target_files->second.get_raw();
        }

        std::string path_c = path;
        config config_(cp->get_product(), cp->get_id(), std::move(raw),
            std::move(hashes), custom_v, std::move(path_c), length);
        auto configs_itr = configs.find(cp->get_product());
        if (configs_itr ==
            configs.end()) { // Product not in configs yet. Create entry
            std::vector<config> configs_on_product = {config_};
            configs.insert(std::pair<std::string, std::vector<config>>(
                cp->get_product(), configs_on_product));
        } else { // Product already exists in configs. Add new config
            configs_itr->second.push_back(config_);
        }
    }

    // Since there have not been errors, we can now update product configs
    for (auto it = std::begin(products_); it != std::end(products_); ++it) {
        auto product_configs = configs.find(it->first);
        if (product_configs != configs.end()) {
            it->second.assign_configs(std::move(product_configs->second));
        } else {
            it->second.assign_configs({});
        }
    }

    targets_version_ = response.get_targets().get_version();
    opaque_backend_state_ = response.get_targets().get_opaque_backend_state();

    return protocol::remote_config_result::success;
}

protocol::remote_config_result client::poll()
{
    if (api_ == nullptr) {
        return protocol::remote_config_result::error;
    }

    auto request = generate_request();

    std::optional<std::string> serialized_request =
        protocol::serialize(std::move(request));
    if (!serialized_request) {
        return protocol::remote_config_result::error;
    }

    auto [result, response_body] =
        api_->get_configs(std::move(serialized_request.value()));
    if (result == protocol::remote_config_result::error) {
        return protocol::remote_config_result::error;
    }
    //@todo improve copies within parse
    auto [parsing_result, response] =
        protocol::parse(std::move(response_body.value()));
    if (parsing_result != protocol::remote_config_parser_result::success) {
        return protocol::remote_config_result::error;
    }

    last_poll_error_ = "";
    //@todo improve copies within process_response
    result = process_response(std::move(response.value()));

    return result;
}

} // namespace dds::remote_config
