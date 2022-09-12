// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <rapidjson/document.h>
#include <string>

#include "common.hpp"
#include "get_configs_request.hpp"

namespace dds::remote_config::protocol {

std::string serialize(const get_configs_request &request);

class serializer_exception : public std::exception {};

} // namespace dds::remote_config::protocol