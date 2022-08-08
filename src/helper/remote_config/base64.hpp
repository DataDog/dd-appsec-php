// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config {

std::string base64_encode(std::string const &s, bool url = false);
std::string base64_encode_pem(std::string const &s);
std::string base64_encode_mime(std::string const &s);

std::string base64_decode(std::string const &s, bool remove_linebreaks = false);
std::string base64_encode(unsigned char const *, size_t len, bool url = false);

} // namespace dds::remote_config
