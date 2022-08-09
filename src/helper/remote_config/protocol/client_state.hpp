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

struct client_state {
public:
    client_state(int targets_version, std::vector<config_state> &&cs,
        bool has_error, std::string &&error, std::string &&backend_client_state)
        : _targets_version(targets_version), _config_states(std::move(cs)),
          _has_error(has_error), _error(std::move(error)),
          _backend_client_state(std::move(backend_client_state)){};
    const int get_targets_version() { return _targets_version; };
    const std::vector<config_state> get_config_states()
    {
        return _config_states;
    };
    const bool get_has_error() { return _has_error; };
    const std::string get_error() { return _error; };
    const std::string get_backend_client_state()
    {
        return _backend_client_state;
    };
    bool operator==(client_state const &b) const
    {
        return this->_targets_version == b._targets_version &&
               this->_config_states == b._config_states &&
               this->_has_error == b._has_error && this->_error == b._error &&
               this->_backend_client_state == b._backend_client_state;
    }

private:
    int _targets_version;
    std::vector<config_state> _config_states;
    //@todo: Test the different combinations of not having error
    bool _has_error;
    std::string _error;
    std::string _backend_client_state;
};

} // namespace dds::remote_config::protocol
