// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <rapidjson/document.h>
#include <string>

#include "get_configs_response.hpp"
#include "common.hpp"

namespace dds::remote_config {

remote_config_result parser(const std::string &body, get_configs_response &output);

} // namespace dds::remote_config