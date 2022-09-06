// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

#include "../product.hpp"
#include "client_state.hpp"
#include "client_tracer.hpp"

namespace dds::remote_config::protocol {

class client {
public:
    client(const std::string &id, const std::vector<std::string> &products,
        const client_tracer &ct, const client_state &cs)
        : id_(id), products_(products), client_tracer_(ct), client_state_(cs){};
    std::string get_id() const { return id_; };
    std::vector<std::string> get_products() const { return products_; };
    [[nodiscard]] client_tracer get_tracer() const { return client_tracer_; };
    client_state get_client_state() const { return client_state_; };
    bool operator==(client const &b) const
    {
        return id_ == b.id_ && products_ == b.products_ &&
               client_tracer_ == b.client_tracer_ &&
               client_state_ == b.client_state_;
    }

private:
    std::string id_;
    std::vector<std::string> products_;
    client_tracer client_tracer_;
    client_state client_state_;
};

} // namespace dds::remote_config::protocol
