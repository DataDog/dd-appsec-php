// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <map>
#include <string>

#include "path.hpp"

namespace dds::remote_config::protocol {

struct targets {
public:
    int get_version() { return _version; };
    std::string get_opaque_backend_state() { return _opaque_backend_state; };
    void set_version(int version) { _version = version; };
    void set_opaque_backend_state(std::string opaque_backend_state)
    {
        _opaque_backend_state = opaque_backend_state;
    };
    void add_path(std::string &plain_path, path &path_object)
    {
        paths.insert(std::pair<std::string, path>(plain_path, path_object));
    }
    std::map<std::string, path> get_paths() { return paths; };
    bool operator==(targets const &b) const
    {
        return this->_version == b._version &&
               std::equal(this->paths.begin(), this->paths.end(),
                   b.paths.begin(), b.paths.end());
    }

private:
    int _version;
    std::string _opaque_backend_state;
    std::map<std::string, path> paths;
};

} // namespace dds::remote_config::protocol
