// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config {

struct cached_target_files_hash {
public:
    cached_target_files_hash(std::string algorithm, std::string hash)
        : algorithm(algorithm), hash(hash){};
    std::string get_algorithm() { return algorithm; };
    std::string get_hash() { return hash; };

private:
    std::string algorithm;
    std::string hash;
};

} // namespace dds::remote_config
