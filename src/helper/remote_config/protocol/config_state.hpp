// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

namespace dds::remote_config::protocol {

struct config_state {
public:
    config_state(std::string id, int version, std::string product)
        : id(id), version(version), product(product){};
    const std::string get_id() { return id; };
    int get_version() { return version; };
    std::string get_product() { return product; };

private:
    std::string id;
    int version;
    std::string product;
};

} // namespace dds::remote_config::protocol
