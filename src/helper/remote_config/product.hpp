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

struct product {
public:
    //@todo add listeners
    explicit product(std::string name) : _name(name){};
    void assign_configs(std::vector<config> configs) { _configs = configs; };
    std::vector<config> get_configs() { return this->_configs; };
    bool operator==(product const &b) const
    {
        return this->_name == b._name && this->_configs == b._configs;
    }
    std::string get_name() { return _name; };

private:
    std::string _name;
    std::vector<config> _configs;
};

} // namespace dds::remote_config
