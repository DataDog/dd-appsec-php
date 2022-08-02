// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <list>
#include <string>

namespace dds::remote_config::protocol {

class client_tracer {
public:
    client_tracer(std::string runtime_id, std::string tracer_version,
        std::string service, std::string env, std::string app_version)
        : runtime_id(runtime_id), tracer_version(tracer_version),
          service(service), env(env), app_version(app_version){};
    std::string get_runtime_id() { return runtime_id; };
    std::string get_tracer_version() { return tracer_version; };
    std::string get_service() { return service; };
    std::string get_env() { return env; };
    std::string get_app_version() { return app_version; };

private:
    std::string runtime_id;
    std::string tracer_version;
    std::string service;
    std::string env;
    std::string app_version;
};

} // namespace dds::remote_config::protocol
