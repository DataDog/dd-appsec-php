// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

struct config_state {
public:
    config_state(std::string &id, int version, std::string &product)
        : _id(id), _version(version), _product(product){};
    std::string get_id() { return _id; };
    [[nodiscard]] int get_version() const { return _version; };
    std::string get_product() { return _product; };
    bool operator==(config_state const &b) const
    {
        return _id == b._id && _version == b._version && _product == b._product;
    }

private:
    std::string _id;
    int _version;
    std::string _product;
};

} // namespace dds::remote_config::protocol
