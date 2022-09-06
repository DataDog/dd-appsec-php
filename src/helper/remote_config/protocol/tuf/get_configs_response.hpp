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
#include "../targets.hpp"

namespace dds::remote_config::protocol {

class get_configs_response {
public:
    get_configs_response(const std::vector<std::string> &client_configs,
        const std::vector<target_file> &target_files, const targets &targets)
        : client_configs_(client_configs), targets_(targets)
    {
        for (auto &target_file : target_files) {
            target_files_.insert(std::pair<std::string, protocol::target_file>(
                target_file.get_path(), target_file));
        }
    }
    [[nodiscard]] std::map<std::string, target_file> get_target_files() const
    {
        return target_files_;
    };
    [[nodiscard]] std::vector<std::string> get_client_configs() const
    {
        return client_configs_;
    };
    [[nodiscard]] targets get_targets() const { return targets_; };

private:
    std::map<std::string, target_file> target_files_;
    std::vector<std::string> client_configs_;
    targets targets_;
};

} // namespace dds::remote_config::protocol
