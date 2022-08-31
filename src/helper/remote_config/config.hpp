// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace dds::remote_config {

struct config {
public:
    //@todo contents should be bytes
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    config(std::string &product, std::string &id, std::string &contents,
        std::map<std::string, std::string> &hashes, int version,
        std::string &path, int length)
        : _product(product), _id(id), _contents(contents), _hashes(hashes),
          _version(version), _path(path), _length(length){};
    bool operator==(config const &b) const
    {
        return this->_product == b._product && this->_id == b._id &&
               this->_contents == b._contents && this->_hashes == b._hashes &&
               this->_version == b._version && this->_path == b._path &&
               this->_length == b._length;
    }
    std::string get_id() { return this->_id; };
    [[nodiscard]] int get_version() const { return this->_version; };
    std::string get_product() { return this->_product; };
    std::string get_contents() { return this->_contents; };
    std::string get_path() { return this->_path; };
    [[nodiscard]] int get_length() const { return this->_length; };
    std::map<std::string, std::string> get_hashes() { return this->_hashes; };

private:
    std::string _product;
    std::string _id;
    std::string _contents;
    std::string _path;
    std::map<std::string, std::string> _hashes;
    int _version;
    int _length;
};

} // namespace dds::remote_config
