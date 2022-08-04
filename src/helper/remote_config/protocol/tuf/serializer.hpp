// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <rapidjson/document.h>
#include <string>

#include "client_get_configs_request.hpp"

namespace dds::remote_config::protocol::tuf {

enum class remote_config_result {
    success,
    error,
};

remote_config_result serialize(
    client_get_configs_request request, std::string &output);

} // namespace dds::remote_config::protocol::tuf