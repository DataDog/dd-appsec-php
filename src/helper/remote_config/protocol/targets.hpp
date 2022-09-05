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

class targets {
public:
    targets(int version, std::string opaque_backend_state,
        std::vector<std::pair<std::string, path>> paths)
        : version_(version), opaque_backend_state_(opaque_backend_state)
    {
        for (auto &pair : paths) { paths_.insert(pair); }
    }
    std::string get_opaque_backend_state() const
    {
        return opaque_backend_state_;
    };
    [[nodiscard]] int get_version() const { return version_; };
    std::map<std::string, path> get_paths() const { return paths_; };
    bool operator==(targets const &b) const
    {
        return version_ == b.version_ &&
               std::equal(paths_.begin(), paths_.end(), b.paths_.begin(),
                   b.paths_.end());
    }

private:
    int version_;
    std::string opaque_backend_state_;
    std::map<std::string, path> paths_;
};

} // namespace dds::remote_config::protocol
