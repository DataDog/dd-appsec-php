// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

namespace dds::remote_config {

struct client_tracer {
public:
    client_tracer(std::string runtime_id, std::string tracer_version,
        std::string service, std::string env, std::string app_version)
        : _runtime_id(runtime_id), _tracer_version(tracer_version),
          _service(service), _env(env), _app_version(app_version){};
    std::string get_runtime_id() { return _runtime_id; };
    std::string get_tracer_version() { return _tracer_version; };
    std::string get_service() { return _service; };
    std::string get_env() { return _env; };
    std::string get_app_version() { return _app_version; };

private:
    std::string _runtime_id;
    std::string _tracer_version;
    std::string _service;
    std::string _env;
    std::string _app_version;
};

} // namespace dds::remote_config
