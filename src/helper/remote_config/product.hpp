// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config.hpp"
#include "exception.hpp"
#include "listener.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace dds::remote_config {

class product {
public:
    product(std::string &&name, product_listener_base *listener)
        : name_(std::move(name)), listener_(listener){};

    void assign_configs(const std::map<std::string, config> &configs);
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
    void update_configs(
        std::map<std::string, dds::remote_config::config> &to_update);
    void unapply_configs(
        std::map<std::string, dds::remote_config::config> &to_unapply);

    std::string name_;
    std::map<std::string, config> configs_;
    product_listener_base *listener_;
};

} // namespace dds::remote_config
