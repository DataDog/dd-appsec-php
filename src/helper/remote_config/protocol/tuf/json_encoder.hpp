// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "get_configs_request.hpp"
#include "get_configs_response.hpp"
#include "parser.hpp"
#include "serializer.hpp"

namespace dds::remote_config::protocol {

struct json_encoder {
public:
    virtual ~json_encoder() = default;
    virtual std::string encode(const get_configs_request &request)
    {
        return protocol::serialize(request);
    }
    virtual get_configs_response decode(const std::string &response)
    {
        return protocol::parse(response);
    }
};
} // namespace dds::remote_config::protocol
