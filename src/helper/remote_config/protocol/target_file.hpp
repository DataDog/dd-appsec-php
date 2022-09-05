// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

namespace dds::remote_config::protocol {

class target_file {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    target_file(std::string &path, std::string &raw) : path_(path), raw_(raw){};
    std::string get_path() { return path_; };
    std::string get_raw() { return raw_; };
    bool operator==(target_file const &b) const
    {
        return path_ == b.path_ && raw_ == b.raw_;
    }

private:
    std::string path_;
    std::string raw_;
};

} // namespace dds::remote_config::protocol
