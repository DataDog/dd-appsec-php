// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>

#include "../cached_target_files.hpp"
#include "../client.hpp"

namespace dds::remote_config::protocol::tuf {

//@todo: Remove client from the name
struct client_get_configs_request {
public:
    client_get_configs_request(
        client client, std::list<cached_target_files> cached_target_files)
        : client(client), cached_target_files(cached_target_files){};

    client get_client() { return client; };
    const std::list<cached_target_files> get_cached_target_files()
    {
        return cached_target_files;
    };

private:
    client client;
    std::list<cached_target_files> cached_target_files;
};

} // namespace dds::remote_config::protocol::tuf
