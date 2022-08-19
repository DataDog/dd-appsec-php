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
        : _custom_v(v), _hashes(hashes), _length(length){};
    int get_custom_v() { return _custom_v; };
    std::map<std::string, std::string> get_hashes() { return _hashes; };
    int get_length() { return _length; };
    bool operator==(path const &b) const
    {
        return this->_custom_v == b._custom_v && this->_hashes == b._hashes &&
               this->_length == b._length;
    }

private:
    int _custom_v;
    std::map<std::string, std::string> _hashes;
    int _length;
};

} // namespace dds::remote_config::protocol
