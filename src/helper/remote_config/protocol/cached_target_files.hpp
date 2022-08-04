// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <vector>

#include "cached_target_file_hash.hpp"

namespace dds::remote_config {

struct cached_target_files {
public:
    cached_target_files(std::string &&path, int length,
        std::vector<cached_target_files_hash> hashes)
        : _path(std::move(path)), _length(length), _hashes(hashes){};
    const std::string get_path() { return _path; };
    const int get_length() { return _length; };
    const std::vector<cached_target_files_hash> get_hashes()
    {
        return _hashes;
    };

private:
    std::string _path;
    int _length;
    std::vector<cached_target_files_hash> _hashes;
};

} // namespace dds::remote_config
