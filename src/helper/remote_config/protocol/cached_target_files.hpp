// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "cached_target_file_hash.hpp"

namespace dds::remote_config::protocol {

struct cached_target_files {
public:
    cached_target_files(std::string &path, int length,
        std::vector<cached_target_files_hash> &hashes)
        : _path(path), _length(length), _hashes(hashes){};
    std::string get_path() { return _path; };
    [[nodiscard]] int get_length() const { return _length; };
    std::vector<cached_target_files_hash> get_hashes() { return _hashes; };
    bool operator==(cached_target_files const &b) const
    {
        return this->_path == b._path && this->_length == b._length &&
               this->_hashes == b._hashes;
    }

private:
    std::string _path;
    int _length;
    std::vector<cached_target_files_hash> _hashes;
};

} // namespace dds::remote_config::protocol
