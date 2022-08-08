// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>
#include <vector>

#include "targets_signed.hpp"

namespace dds::remote_config {

struct targets {
public:
    targets_signed get_target_signed() { return _target_signed; };

private:
    targets_signed _target_signed;
};

} // namespace dds::remote_config
