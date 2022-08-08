// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <iostream>

namespace dds::remote_config {

struct targets_signed {
public:
    const int get_version() { return _version; };
    void set_version(int version) { _version = version; };

private:
    int _version;
};

} // namespace dds::remote_config
