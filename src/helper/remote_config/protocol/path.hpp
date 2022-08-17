// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

namespace dds::remote_config::protocol {

struct path {
public:
    path(int v, std::string &&hash, int length)
        : _custom_v(v), _hash(std::move(hash)), _length(length){};
    int get_custom_v() { return _custom_v; };
    std::string get_hash() { return _hash; };
    int get_length() { return _length; };
    bool operator==(path const &b) const
    {
        return this->_custom_v == b._custom_v && this->_hash == b._hash &&
               this->_length == b._length;
    }

private:
    int _custom_v;
    std::string _hash;
    int _length;
};

} // namespace dds::remote_config::protocol
