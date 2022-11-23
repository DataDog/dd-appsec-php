// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include "exception.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace dds::remote_config {

class product_listener_base {
public:
    product_listener_base() = default;
    product_listener_base(const product_listener_base &) = default;
    product_listener_base(product_listener_base &&) = default;
    product_listener_base &operator=(const product_listener_base &) = default;
    product_listener_base &operator=(product_listener_base &&) = default;
    virtual ~product_listener_base() = default;

    virtual void on_update(const config &config) = 0;
    virtual void on_unapply(const config &config) = 0;
};

class product {
public:
    product(std::string &&name, product_listener_base *listener)
        : name_(std::move(name)), listener_(listener){};

    void update_configs(std::unordered_map<std::string, config> &to_update)
    {
        for (auto &[name, config] : to_update) {
            config.apply_state =
                protocol::config_state::applied_state::UNACKNOWLEDGED;
            if (listener_ == nullptr) {
                continue;
            }
            try {
                listener_->on_update(config);
                config.apply_state =
                    protocol::config_state::applied_state::ACKNOWLEDGED;
                config.apply_error = "";
            } catch (error_applying_config &e) {
                config.apply_state =
                    protocol::config_state::applied_state::ERROR;
                config.apply_error = e.what();
            }
        }
    }

    void unapply_configs(std::map<std::string, config> &to_unapply)
    {
        for (auto &[path, conf] : to_unapply) {
            if (listener_ == nullptr) {
                continue;
            }
            try {
                listener_->on_unapply(conf);
                conf.apply_state =
                    protocol::config_state::applied_state::ACKNOWLEDGED;
                conf.apply_error = "";
            } catch (error_applying_config &e) {
                conf.apply_state = protocol::config_state::applied_state::ERROR;
                conf.apply_error = e.what();
            }
        }
    }

    void assign_configs(const std::map<std::string, config> &configs)
    {
        std::unordered_map<std::string, config> to_update;
        std::unordered_map<std::string, config> to_keep;
        // determine what each config given is
        for (const auto &[name, config] : configs) {
            auto previous_config = configs_.find(name);
            if (previous_config == configs_.end()) { // New config
                to_update.emplace(name, config);
            } else { // Already existed
                if (config.hashes ==
                    previous_config->second.hashes) { // No changes in config
                    to_keep.emplace(name, previous_config->second);
                } else { // Config updated
                    to_update.emplace(name, config);
                }
                // configs_ at the end of this loop will contain only configs
                // which have to be unapply. This one has been classified as
                // something else and therefore, it has to be removed
                configs_.erase(previous_config);
            }
        }

        update_configs(to_update);
        unapply_configs(configs_);
        to_keep.merge(to_update);

        // Save new state of configs
        std::map<std::string, config> to_keep_ordered_map(
            to_keep.begin(), to_keep.end());
        configs_ = std::move(to_keep_ordered_map);
    };
    [[nodiscard]] const std::map<std::string, config> &get_configs() const
    {
        return configs_;
    };
    bool operator==(product const &b) const
    {
        return name_ == b.name_ && configs_ == b.configs_;
    }
    [[nodiscard]] const std::string &get_name() const { return name_; }

protected:
    std::string name_;
    std::map<std::string, config> configs_;
    product_listener_base *listener_;
};

} // namespace dds::remote_config
