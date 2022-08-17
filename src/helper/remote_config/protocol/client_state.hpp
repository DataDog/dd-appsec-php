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
    client_state(int targets_version, std::vector<config_state> &cs,
        bool has_error, std::string &error, std::string &backend_client_state)
        : _targets_version(targets_version), _config_states(cs),
          _has_error(has_error), _error(error),
          _backend_client_state(backend_client_state){};
    int get_targets_version() { return _targets_version; };
    std::vector<config_state> get_config_states() { return _config_states; };
    bool get_has_error() { return _has_error; };
    std::string get_error() { return _error; };
    std::string get_backend_client_state() { return _backend_client_state; };
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
