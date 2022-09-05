// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "cached_target_file_hash.hpp"

namespace dds::remote_config::protocol {

class cached_target_files {
public:
    cached_target_files(std::string &path, int length,
        std::vector<cached_target_files_hash> &hashes)
        : path_(path), length_(length), hashes_(hashes){};
    std::string get_path() const { return path_; };
    [[nodiscard]] int get_length() const { return length_; };
    std::vector<cached_target_files_hash> get_hashes() const
    {
        return hashes_;
    };
    bool operator==(cached_target_files const &b) const
    {
        return path_ == b.path_ && length_ == b.length_ && hashes_ == b.hashes_;
    }

private:
    std::string path_;
    int length_;
    std::vector<cached_target_files_hash> hashes_;
};

} // namespace dds::remote_config::protocol
