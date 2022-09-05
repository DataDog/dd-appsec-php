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

class get_configs_request {
public:
    get_configs_request(client &arg_client,
        std::vector<cached_target_files> &arg_cached_target_files)
        : client_(arg_client), cached_target_files_(arg_cached_target_files){};

    client get_client() { return client_; };
    bool operator==(get_configs_request const &a) const
    {
        return client_ == a.client_ &&
               cached_target_files_ == a.cached_target_files_;
    }
    std::vector<cached_target_files> get_cached_target_files()
    {
        return cached_target_files_;
    };

private:
    client client_;
    std::vector<cached_target_files> cached_target_files_;
};

} // namespace dds::remote_config::protocol
