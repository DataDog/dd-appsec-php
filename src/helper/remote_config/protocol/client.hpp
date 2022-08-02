// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

namespace dds::remote_config::protocol {

enum product { LIVE_DEBUGGING, ASM_DD, FEATURES };

class client {
public:
    client(std::string id, std::list<product> products)
        : id(id), products(products){};
    std::string get_id() { return this->id; };
    std::list<product> get_products() { return this->products; };

private:
    std::string id;
    std::list<product> products;
};

} // namespace dds::remote_config::protocol
