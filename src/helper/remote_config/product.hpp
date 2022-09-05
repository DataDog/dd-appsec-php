// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace dds::remote_config {

class product_listener_abstract {
public:
    virtual void on_update(std::vector<config> configs) = 0;
    virtual void on_unapply(std::vector<config> configs) = 0;
};

class product_listener : product_listener_abstract {
public:
    void on_update(std::vector<config> configs) override{};
    void on_unapply(std::vector<config> configs) override{};
};

struct product {
public:
    explicit product(
        std::string &name, std::vector<product_listener *> &listeners)
        : name_(name), listeners_(listeners){};
    void assign_configs(std::vector<config> &configs)
    {
        std::vector<config> to_update;
        std::vector<config> to_keep;

        for (config config_ : configs) {
            auto previous_config = std::find_if(
                configs_.begin(), configs_.end(), [&config_](config c) {
                    return c.get_id() == config_.get_id();
                });
            if (previous_config == configs_.end()) { // New config
                to_update.push_back(config_);
            } else { // Already existed
                if (config_.get_hashes() ==
                    previous_config->get_hashes()) { // No changes in config
                    to_keep.push_back(config_);
                } else { // Config updated
                    to_update.push_back(config_);
                }
                configs_.erase(previous_config);
            }
        }

        for (product_listener *l : listeners_) {
            l->on_update(to_update);
            l->on_unapply(configs_);
        }
        to_keep.insert(to_keep.end(), to_update.begin(), to_update.end());

        configs_ = to_keep;
    };
    std::vector<config> get_configs() { return configs_; };
    bool operator==(product const &b) const
    {
        return name_ == b.name_ && configs_ == b.configs_;
    }
    std::string get_name() { return name_; };

private:
    std::string name_;
    std::vector<config> configs_;
    std::vector<product_listener *> listeners_;
};

} // namespace dds::remote_config
