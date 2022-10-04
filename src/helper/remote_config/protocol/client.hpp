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

#define BIT(x) (1 << x)

namespace dds::remote_config::protocol {

enum class capabilities_e : int {
    RESERVED = BIT(0),
    ASM_ACTIVATION = BIT(1),
    ASM_IP_BLOCKING = BIT(2),
    ASM_DD_RULES = BIT(3)
};

struct client {
    std::string id;
    std::vector<std::string> products;
    protocol::client_tracer client_tracer;
    protocol::client_state client_state;
    std::byte capabilities;

    void set_capabilities(std::vector<capabilities_e> cs)
    {
        for (const auto &capability : cs) {
            capabilities |= (std::byte)capability;
        }
    }
};

inline bool operator==(const client &rhs, const client &lhs)
{
    return rhs.id == lhs.id && rhs.products == lhs.products &&
           rhs.client_tracer == lhs.client_tracer &&
           rhs.client_state == lhs.client_state &&
           rhs.capabilities == lhs.capabilities;
}

} // namespace dds::remote_config::protocol
