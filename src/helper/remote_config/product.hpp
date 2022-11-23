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

class error_applying_config : public std::exception {
private:
    std::string message;

public:
    explicit error_applying_config(std::string &&msg) : message(std::move(msg))
    {}
    std::string what() { return message; }
};

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
                    to_keep.emplace(config.first, previous_config->second);
                } else { // Config updated
                    to_update.emplace(config.first, config.second);
                }
                configs_.erase(previous_config);
            }
        }

        std::map<std::string, config>::iterator update_it;
        for (update_it = to_update.begin(); update_it != to_update.end();
             update_it++) {
            update_it->second.apply_state =
                protocol::config_state_applied_state::UNACKNOWLEDGED;
            if (listener_ != nullptr) {
                try {

                    listener_->on_update(update_it->second);
                    update_it->second.apply_state =
                        protocol::config_state_applied_state::ACKNOWLEDGED;
                    update_it->second.apply_error = "";
                } catch (error_applying_config &e) {
                    update_it->second.apply_state =
                        protocol::config_state_applied_state::ERROR;
                    update_it->second.apply_error = e.what();
                }
            }
        }

        for (auto &[path, conf] : configs_) {
            if (listener_ != nullptr) {
                try {
                    listener_->on_unapply(conf);
                    conf.apply_state =
                        protocol::config_state_applied_state::ACKNOWLEDGED;
                    conf.apply_error = "";
                } catch (error_applying_config &e) {
                    conf.apply_state =
                        protocol::config_state_applied_state::ERROR;
                    conf.apply_error = e.what();
                }
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
