// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "cached_target_file_hash.hpp"

namespace dds::remote_config::protocol {

struct target_file {
public:
    target_file(std::string &path, std::string &raw) : _path(path), _raw(raw){};
    std::string get_path() { return _path; };
    std::string get_raw() { return _raw; };
    bool operator==(target_file const &b) const
    {
        return this->_path == b._path && this->_raw == b._raw;
    }

private:
    std::string _path;
    std::string _raw;
};

} // namespace dds::remote_config::protocol
