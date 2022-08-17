// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

namespace dds::remote_config {

struct config {
public:
    //@todo contents should be bytes
    //@todo hash should be map<string, string>
    config(std::string &product, std::string &id, std::string &contents,
        std::string hash, int version)
        : _product(product), _id(id), _contents(contents), _hash(hash),
          _version(version){};
    bool operator==(config const &b) const
    {
        return this->_product == b._product && this->_id == b._id &&
               this->_contents == b._contents && this->_hash == b._hash &&
               this->_version == b._version;
    }
    std::string get_id() { return this->_id; };
    int get_version() const { return this->_version; };
    std::string get_product() { return this->_product; };

private:
    std::string _product;
    std::string _id;
    std::string _contents;
    std::string _hash;
    int _version;
};

} // namespace dds::remote_config
