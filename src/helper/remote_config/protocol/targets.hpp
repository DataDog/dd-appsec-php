// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

namespace dds::remote_config {

struct targets {
public:
    const int64_t get_version() { return _version; };
    void set_version(int64_t version) { _version = version; };

private:
    int64_t _version;
};

} // namespace dds::remote_config
