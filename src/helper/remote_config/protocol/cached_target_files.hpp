// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

#include "cached_target_file_hash.hpp"

namespace dds::remote_config::protocol {

struct cached_target_files {
public:
    cached_target_files(std::string path, int length,
        std::list<cached_target_files_hash> hashes)
        : path(path), length(length), hashes(hashes){};
    const std::string get_path() { return path; };
    const int get_length() { return length; };
    const std::list<cached_target_files_hash> get_hashes() { return hashes; };

private:
    std::string path;
    int length;
    std::list<cached_target_files_hash> hashes;
};

} // namespace dds::remote_config::protocol
