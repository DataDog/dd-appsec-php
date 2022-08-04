// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "config_state.hpp"
#include <list>
#include <string>

namespace dds::remote_config {

struct client_state {
public:
    client_state(int targets_version, std::list<config_state> config_states,
        bool has_error, std::string error, std::string backend_client_state)
        : targets_version(targets_version), config_states(config_states),
          has_error(has_error), error(error),
          backend_client_state(backend_client_state){};
    int get_targets_version() { return targets_version; };
    std::list<config_state> get_config_states() { return config_states; };
    bool get_has_error() { return has_error; };
    std::string get_error() { return error; };
    std::string get_backend_client_state() { return backend_client_state; };

private:
    int targets_version;
    std::list<config_state> config_states;
    //@todo: Test the different combinations of not having error
    bool has_error;
    std::string error;
    std::string backend_client_state;
};

} // namespace dds::remote_config
