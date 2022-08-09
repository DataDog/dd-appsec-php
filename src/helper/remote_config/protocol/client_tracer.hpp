// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

struct client_tracer {
public:
    client_tracer(std::string &&runtime_id, std::string &&tracer_version,
        std::string &&service, std::string &&env, std::string &&app_version)
        : _runtime_id(std::move(runtime_id)),
          _tracer_version(std::move(tracer_version)),
          _service(std::move(service)), _env(std::move(env)),
          _app_version(std::move(app_version)){};
    const std::string get_runtime_id() { return _runtime_id; };
    const std::string get_tracer_version() { return _tracer_version; };
    const std::string get_service() { return _service; };
    const std::string get_env() { return _env; };
    const std::string get_app_version() { return _app_version; };
    bool operator==(client_tracer const &b) const
    {
        return this->_runtime_id == b._runtime_id &&
               this->_tracer_version == b._tracer_version &&
               this->_service == b._service && this->_env == b._env &&
               this->_app_version == b._app_version;
    }

private:
    std::string _runtime_id;
    std::string _tracer_version;
    std::string _service;
    std::string _env;
    std::string _app_version;
};

} // namespace dds::remote_config::protocol
