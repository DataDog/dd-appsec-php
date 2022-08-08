// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

namespace dds::remote_config {

struct path {
public:
    path(int64_t v, std::string &&hash, int64_t length)
        : _custom_v(v), _hash(std::move(hash)), _length(length){};
    const int64_t get_custom_v() { return _custom_v; };
    const std::string get_hash() { return _hash; };
    const int64_t get_length() { return _length; };

private:
    int64_t _custom_v;
    std::string _hash;
    int64_t _length;
};

} // namespace dds::remote_config
