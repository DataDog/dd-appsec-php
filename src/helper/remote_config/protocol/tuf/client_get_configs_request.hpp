// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "../client.hpp"

namespace dds::remote_config::protocol::tuf {

class client_get_configs_request {
public:
    client_get_configs_request(client client) : client(client){};

    client get_client() { return client; };

private:
    client client;
};

} // namespace dds::remote_config::protocol::tuf
