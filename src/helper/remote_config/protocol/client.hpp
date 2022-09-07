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
    client(std::string &&id, std::vector<std::string> &&products,
        client_tracer &&ct, client_state &&cs)
        : id_(std::move(id)), products_(std::move(products)),
          client_tracer_(std::move(ct)), client_state_(std::move(cs)){};
    [[nodiscard]] std::string get_id() const { return id_; };
    [[nodiscard]] std::vector<std::string> get_products() const
    {
        return products_;
    };
    [[nodiscard]] client_tracer get_tracer() const { return client_tracer_; };
    [[nodiscard]] client_state get_client_state() const
    {
        return client_state_;
    };
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
