// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

struct cached_target_files_hash {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    cached_target_files_hash(std::string &algorithm, std::string &hash)
        : _algorithm(algorithm), _hash(hash){};
    std::string get_algorithm() { return _algorithm; };
    std::string get_hash() { return _hash; };
    bool operator==(cached_target_files_hash const &b) const
    {
        return this->_algorithm == b._algorithm && this->_hash == b._hash;
    }

private:
    std::string _algorithm;
    std::string _hash;
};

} // namespace dds::remote_config::protocol
