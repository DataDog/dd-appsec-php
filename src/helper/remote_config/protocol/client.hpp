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

struct client {
public:
    client(std::string &id, std::vector<std::string> &products,
        client_tracer ct, client_state cs)
        : _id(id), _products(products), _client_tracer(ct), _client_state(cs){};
    std::string get_id() { return _id; };
    std::vector<std::string> get_products() { return _products; };
    client_tracer get_tracer() { return _client_tracer; };
    client_state get_client_state() { return _client_state; };
    bool operator==(client const &b) const
    {
        return this->_id == b._id && this->_products == b._products &&
               this->_client_tracer == b._client_tracer &&
               this->_client_state == b._client_state;
    }

private:
    std::string _id;
    std::vector<std::string> _products;
    client_tracer _client_tracer;
    client_state _client_state;
};

} // namespace dds::remote_config::protocol
