// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

class cached_target_files_hash {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    cached_target_files_hash(const std::string &algorithm, std::string &hash)
        : algorithm_(algorithm), hash_(hash){};
    std::string get_algorithm() const { return algorithm_; };
    std::string get_hash() const { return hash_; };
    bool operator==(cached_target_files_hash const &b) const
    {
        return algorithm_ == b.algorithm_ && hash_ == b.hash_;
    }

private:
    std::string algorithm_;
    std::string hash_;
};

} // namespace dds::remote_config::protocol
