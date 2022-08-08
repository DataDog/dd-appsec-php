// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <map>
#include <string>

#include "path.hpp"

namespace dds::remote_config {

struct targets {
public:
    const int64_t get_version() { return _version; };
    void set_version(int64_t version) { _version = version; };
    void add_path(std::string &&plain_path, path &&path_object)
    {
        paths.insert(std::pair<std::string, path>(
            std::move(plain_path), std::move(path_object)));
    }
    std::map<std::string, path> get_paths() { return paths; };

private:
    int64_t _version;
    std::map<std::string, path> paths;
};

} // namespace dds::remote_config
