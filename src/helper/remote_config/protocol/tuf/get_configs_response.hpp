// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <vector>

#include "../cached_target_files.hpp"
#include "../client.hpp"
#include "../target_file.hpp"

namespace dds::remote_config {

struct get_configs_response {
public:
    const std::vector<target_file> get_target_files() { return _target_files; };
    void add_target_file(target_file tf) { _target_files.push_back(tf); };

private:
    std::vector<target_file> _target_files;
};

} // namespace dds::remote_config
