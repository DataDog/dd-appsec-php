// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "api.hpp"

namespace dds::remote_config {

class http_api : api {
public:
    protocol::remote_config_result get_configs(
        std::string request, std::string &response_body)
    {
        return protocol::remote_config_result::success;
    };
};

} // namespace dds::remote_config
