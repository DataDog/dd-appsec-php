// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "../cached_target_files.hpp"
#include "../client.hpp"

namespace dds::remote_config::protocol {

struct get_configs_request {
public:
    get_configs_request(client &arg_client,
        std::vector<cached_target_files> &arg_cached_target_files)
        : _client(arg_client), _cached_target_files(arg_cached_target_files){};

    client get_client() { return _client; };
    bool operator==(get_configs_request const &a) const
    {
        return _client == a._client &&
               _cached_target_files == a._cached_target_files;
    }
    std::vector<cached_target_files> get_cached_target_files()
    {
        return _cached_target_files;
    };

private:
    client _client;
    std::vector<cached_target_files> _cached_target_files;
};

} // namespace dds::remote_config::protocol
