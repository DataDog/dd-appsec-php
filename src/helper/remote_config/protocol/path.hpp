// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace dds::remote_config::protocol {

struct path {
public:
    path(int v, std::map<std::string, std::string> &hashes, int length)
        : custom_v_(v), hashes_(hashes), length_(length){};
    [[nodiscard]] int get_custom_v() const { return custom_v_; };
    std::map<std::string, std::string> get_hashes() { return hashes_; };
    [[nodiscard]] int get_length() const { return length_; };
    bool operator==(path const &b) const
    {
        return custom_v_ == b.custom_v_ && hashes_ == b.hashes_ &&
               length_ == b.length_;
    }

private:
    int custom_v_;
    std::map<std::string, std::string> hashes_;
    int length_;
};

} // namespace dds::remote_config::protocol
