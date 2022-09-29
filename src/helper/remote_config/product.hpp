// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
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
    explicit product(std::string &&name, product_listener_base *listener)
        : name_(std::move(name)), listener_(listener){};
    void assign_configs(const std::map<std::string, config> &configs)
    {
        std::map<std::string, config> to_update;
        std::map<std::string, config> to_keep;

        for (const auto &config : configs) {
            auto previous_config = configs_.find(config.first);
            if (previous_config == configs_.end()) { // New config
                to_update.emplace(config.first, config.second);
            } else { // Already existed
                if (config.second.hashes ==
                    previous_config->second.hashes) { // No changes in config
                    to_keep.emplace(config.first, config.second);
                } else { // Config updated
                    to_update.emplace(config.first, config.second);
                }
                configs_.erase(previous_config);
            }
        }

        for (auto &[path, conf] : to_update) {
            if (listener_) {
                listener_->on_update(conf);
                conf.apply_state =
                    protocol::config_state_applied_state::ACKNOWLEDGED;
                conf.apply_error = "";
            }
        }

        for (auto &[path, conf] : configs_) {
            if (listener_) {
                listener_->on_unapply(conf);
                conf.apply_state =
                    protocol::config_state_applied_state::ACKNOWLEDGED;
                conf.apply_error = "";
            }
        }

        to_keep.merge(to_update);

        configs_ = std::move(to_keep);
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

private:
    std::string name_;
    std::map<std::string, config> configs_;
    product_listener_base *listener_;
};

} // namespace dds::remote_config
