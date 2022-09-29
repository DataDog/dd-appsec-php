// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "protocol/config_state.hpp"
#include <map>
#include <string>
#include <vector>

namespace dds::remote_config {

struct config {
    std::string product;
    std::string id;
    std::string contents;
    std::string path;
    std::map<std::string, std::string> hashes;
    int version;
    int length;
    protocol::config_state_applied_state apply_state;
    std::string apply_error;
};

inline bool operator==(const config &rhs, const config &lhs)
{
    return rhs.product == lhs.product && rhs.id == lhs.id &&
           rhs.contents == lhs.contents && rhs.hashes == lhs.hashes &&
           rhs.version == lhs.version && rhs.path == lhs.path &&
           rhs.length == lhs.length && rhs.apply_state == lhs.apply_state &&
           rhs.apply_error == lhs.apply_error;
}

} // namespace dds::remote_config
