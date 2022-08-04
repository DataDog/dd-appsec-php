// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "../cached_target_files.hpp"
#include "../client.hpp"

namespace dds::remote_config {

struct get_configs_request {
public:
    get_configs_request(
        client client, std::vector<cached_target_files> &&cached_target_files)
        : client(client), cached_target_files(std::move(cached_target_files)){};

    client get_client() { return client; };
    const std::vector<cached_target_files> get_cached_target_files()
    {
        return cached_target_files;
    };

private:
    client client;
    std::vector<cached_target_files> cached_target_files;
};

} // namespace dds::remote_config
