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
        : _name(name), _listeners(listeners){};
    void assign_configs(std::vector<config> &configs)
    {
        std::vector<config> to_update;
        std::vector<config> to_keep;

        for (config _config : configs) {
            auto previous_config = std::find_if(
                _configs.begin(), _configs.end(), [&_config](config c) {
                    return c.get_id() == _config.get_id();
                });
            if (previous_config == _configs.end()) { // New config
                to_update.push_back(_config);
            } else { // Already existed
                if (_config.get_hashes() ==
                    previous_config->get_hashes()) { // No changes in config
                    to_keep.push_back(_config);
                } else { // Config updated
                    to_update.push_back(_config);
                }
                _configs.erase(previous_config);
            }
        }

        for (product_listener *l : _listeners) {
            l->on_update(to_update);
            l->on_unapply(_configs);
        }
        to_keep.insert(to_keep.end(), to_update.begin(), to_update.end());

        _configs = to_keep;
    };
    std::vector<config> get_configs() { return this->_configs; };
    bool operator==(product const &b) const
    {
        return this->_name == b._name && this->_configs == b._configs;
    }
    std::string get_name() { return _name; };

private:
    std::string _name;
    std::vector<config> _configs;
    std::vector<product_listener *> _listeners;
};

} // namespace dds::remote_config
