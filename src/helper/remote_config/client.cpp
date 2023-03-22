// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client.hpp"
#include "exception.hpp"
#include "product.hpp"
#include "protocol/tuf/parser.hpp"
#include "protocol/tuf/serializer.hpp"
#include "remote_config/asm_data_listener.hpp"
#include "remote_config/asm_dd_listener.hpp"
#include "remote_config/asm_features_listener.hpp"
#include "remote_config/asm_listener.hpp"
#include <algorithm>
#include <regex>
#include <spdlog/spdlog.h>
#include <vector>

namespace dds::remote_config {

config_path config_path::from_path(const std::string &path)
{
    static const std::regex regex(
        "^(datadog/\\d+|employee)/([^/]+)/([^/]+)/[^/]+$");

    std::smatch base_match;
    if (!std::regex_match(path, base_match, regex) || base_match.size() < 4) {
        throw invalid_path();
    }

    return config_path{base_match[3].str(), base_match[2].str()};
}

client::client(std::unique_ptr<http_api> &&arg_api, service_identifier sid,
    remote_config::settings settings, const std::vector<product> &products)
    : api_(std::move(arg_api)), id_(dds::generate_random_uuid()),
      sid_(std::move(sid)), settings_(std::move(settings))
{
    for (auto const &product : products) {
        products_.insert(std::pair<std::string, remote_config::product>(
            product.get_name(), product));
        capabilities_ |= product.get_capabilities();
    }
}

client::ptr client::from_settings(const service_identifier &sid,
    const remote_config::settings &settings,
    std::optional<bool> client_enable_configuration,
    const std::shared_ptr<dds::service_config> &service_config,
    const std::shared_ptr<dds::engine> &engine_ptr, bool rules_file_set)
{
    if (!settings.enabled) {
        return {};
    }

    std::vector<remote_config::product> products = {};
    if (!client_enable_configuration.has_value()) {
        auto asm_features_listener =
            std::make_shared<remote_config::asm_features_listener>(
                service_config);
        products.emplace_back(asm_features_listener);
    }
    if (!rules_file_set) {
        auto asm_data_listener =
            std::make_shared<remote_config::asm_data_listener>(engine_ptr);
        auto asm_dd_listener = std::make_shared<remote_config::asm_dd_listener>(
            engine_ptr, dds::engine_settings::default_rules_file());
        auto asm_listener =
            std::make_shared<remote_config::asm_listener>(engine_ptr);

        products.emplace_back(asm_data_listener);
        products.emplace_back(asm_dd_listener);
        products.emplace_back(asm_listener);
    }

    if (products.empty()) {
        return {};
    }

    // TODO runtime_id will be send by the extension when the extension can get
    // it from the profiler. When that happen, this wont be needed
    auto sid_copy = sid;
    if (sid_copy.runtime_id.empty()) {
        sid_copy.runtime_id = generate_random_uuid();
    }

    return std::make_unique<client>(std::make_unique<http_api>(settings.host,
                                        std::to_string(settings.port)),
        std::move(sid_copy), settings, std::move(products));
}

[[nodiscard]] protocol::get_configs_request client::generate_request() const
{
    std::vector<protocol::config_state> config_states;
    std::vector<protocol::cached_target_files> files;

    for (const auto &[product_name, product] : products_) {
        // State
        const auto configs_on_product = product.get_configs();
        for (const auto &[id, config] : configs_on_product) {
            config_states.push_back({config.id, config.version, config.product,
                config.apply_state, config.apply_error});

            std::vector<protocol::cached_target_files_hash> hashes;
            hashes.reserve(config.hashes.size());
            for (auto const &[algo, hash_sting] : config.hashes) {
                hashes.push_back({algo, hash_sting});
            }
            files.push_back({config.path, config.length, std::move(hashes)});
        }
    }

    const protocol::client_tracer ct{sid_.runtime_id, sid_.tracer_version,
        sid_.service, sid_.env, sid_.app_version};

    const protocol::client_state cs{targets_version_, config_states,
        !last_poll_error_.empty(), last_poll_error_, opaque_backend_state_};
    std::vector<std::string> products_str;
    products_str.reserve(products_.size());
    for (const auto &[product_name, product] : products_) {
        products_str.push_back(product_name);
    }
    protocol::client protocol_client = {
        id_, products_str, ct, cs, capabilities_};

    return {std::move(protocol_client), std::move(files)};
};

bool client::process_response(const protocol::get_configs_response &response)
{
    if (!response.targets.has_value()) {
        return true;
    }

    const std::unordered_map<std::string, protocol::path> paths_on_targets =
        response.targets->paths;
    const std::unordered_map<std::string, protocol::target_file> target_files =
        response.target_files;
    std::unordered_map<std::string, std::unordered_map<std::string, config>>
        configs;
    for (const std::string &path : response.client_configs) {
        try {
            auto cp = config_path::from_path(path);

            // Is path on targets?
            auto path_itr = paths_on_targets.find(path);
            if (path_itr == paths_on_targets.end()) {
                // Not found
                last_poll_error_ = "missing config " + path + " in targets";
                return false;
            }
            auto length = path_itr->second.length;
            std::unordered_map<std::string, std::string> hashes =
                path_itr->second.hashes;
            int custom_v = path_itr->second.custom_v;

            // Is product on the requested ones?
            auto product = products_.find(cp.product);
            if (product == products_.end()) {
                // Not found
                last_poll_error_ = "received config " + path +
                                   " for a product that was not requested";
                return false;
            }

            // Is path on target_files?
            auto path_in_target_files = target_files.find(path);
            std::string raw;
            if (path_in_target_files == target_files.end()) {
                // Check if file in cache
                auto configs_on_product = product->second.get_configs();
                auto config_itr = std::find_if(configs_on_product.begin(),
                    configs_on_product.end(), [&path, &hashes](auto &pair) {
                        return pair.second.path == path &&
                               pair.second.hashes == hashes;
                    });

                if (config_itr == configs_on_product.end()) {
                    // Not found
                    last_poll_error_ = "missing config " + path +
                                       " in target files and in cache files";
                    return false;
                }

                raw = config_itr->second.contents;
                length = config_itr->second.length;
                custom_v = config_itr->second.version;
            } else {
                raw = path_in_target_files->second.raw;
            }

            const std::string path_c = path;
            config const config_ = {
                cp.product, cp.id, raw, path_c, hashes, custom_v, length};
            auto configs_itr = configs.find(cp.product);
            if (configs_itr ==
                configs.end()) { // Product not in configs yet. Create entry
                std::unordered_map<std::string, config> configs_on_product;
                configs_on_product.emplace(cp.id, config_);
                configs.insert(std::pair<std::string,
                    std::unordered_map<std::string, config>>(
                    cp.product, configs_on_product));
            } else { // Product already exists in configs. Add new config
                configs_itr->second.emplace(cp.id, config_);
            }
        } catch (invalid_path &e) {
            last_poll_error_ = "error parsing path " + path;
            return false;
        }
    }

    // Since there have not been errors, we can now update product configs
    for (auto &[name, product] : products_) {
        const auto product_configs = configs.find(name);
        if (product_configs != configs.end()) {
            product.assign_configs(product_configs->second);
        } else {
            product.assign_configs({});
        }
    }

    targets_version_ = response.targets->version;
    opaque_backend_state_ = response.targets->opaque_backend_state;

    return true;
}

bool client::is_remote_config_available()
{
    auto response_body = api_->get_info();
    try {
        SPDLOG_TRACE("Received info response: {}", response_body);
        auto response = protocol::parse_info(response_body);

        return std::find(response.endpoints.begin(), response.endpoints.end(),
                   "/v0.7/config") != response.endpoints.end();
    } catch (protocol::parser_exception &e) {
        SPDLOG_ERROR("Error parsing info response - {}", e.what());
        return false;
    }
}

bool client::poll()
{
    if (api_ == nullptr) {
        return false;
    }

    auto request = generate_request();

    std::string serialized_request;
    try {
        serialized_request = protocol::serialize(request);
        SPDLOG_TRACE("Sending request: {}", serialized_request);
    } catch (protocol::serializer_exception &e) {
        return false;
    }

    auto response_body = api_->get_configs(std::move(serialized_request));

    try {
        SPDLOG_TRACE("Received response: {}", response_body);
        auto response = protocol::parse(response_body);
        last_poll_error_.clear();
        return process_response(response);
    } catch (protocol::parser_exception &e) {
        SPDLOG_ERROR("Error parsing remote config response - {}", e.what());
        return false;
    }
}

} // namespace dds::remote_config
