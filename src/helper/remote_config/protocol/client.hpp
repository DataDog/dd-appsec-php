// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

#include "client_state.hpp"
#include "client_tracer.hpp"

namespace dds::remote_config::protocol {

enum class product { live_debugging, asm_dd, features };

struct client {
public:
    client(std::string id, std::list<product> products,
        client_tracer client_tracer, client_state client_state)
        : id(id), products(products), client_tracer(client_tracer),
          client_state(client_state){};
    std::string get_id() { return this->id; };
    std::list<product> get_products() { return this->products; };
    client_tracer get_tracer() { return this->client_tracer; };
    client_state get_client_state() { return this->client_state; };

private:
    std::string id;
    std::list<product> products;
    client_tracer client_tracer;
    client_state client_state;
};

} // namespace dds::remote_config::protocol
