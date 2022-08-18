// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace dds::remote_config {

class product_listener_abstract {
public:
    virtual void on_update(std::vector<remote_config::config> configs) = 0;
    //@todo implement the unapply call
    virtual void on_unapply(std::vector<remote_config::config> configs) = 0;
};

class product_listener : product_listener_abstract {
public:
    void on_update(std::vector<remote_config::config> configs) override{};
    void on_unapply(std::vector<remote_config::config> configs) override{};
};

struct product {
public:
    explicit product(
        std::string name, std::vector<product_listener *> &listeners)
        : _name(name), _listeners(listeners){};
    void assign_configs(std::vector<config> configs)
    {
        _configs = configs;
        for (product_listener *l : _listeners) { l->on_update(_configs); }
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
