// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config_state.hpp"
#include <string>
#include <vector>

namespace dds::remote_config::protocol {

class client_state {
public:
    client_state(int targets_version, const std::vector<config_state> &cs,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        bool has_error, const std::string &error,
        const std::string &backend_client_state)
        : targets_version_(targets_version), config_states_(std::move(cs)),
          has_error_(has_error), error_(std::move(error)),
          backend_client_state_(std::move(backend_client_state)){};
    [[nodiscard]] int get_targets_version() const { return targets_version_; };
    [[nodiscard]] std::vector<config_state> get_config_states() const
    {
        return config_states_;
    };
    [[nodiscard]] bool get_has_error() const { return has_error_; };
    [[nodiscard]] std::string get_error() const { return error_; };
    [[nodiscard]] std::string get_backend_client_state() const
    {
        return backend_client_state_;
    };
    bool operator==(client_state const &b) const
    {
        return targets_version_ == b.targets_version_ &&
               config_states_ == b.config_states_ &&
               has_error_ == b.has_error_ && error_ == b.error_ &&
               backend_client_state_ == b.backend_client_state_;
    }

private:
    int targets_version_;
    std::vector<config_state> config_states_;
    bool has_error_;
    std::string error_;
    std::string backend_client_state_;
};

} // namespace dds::remote_config::protocol
