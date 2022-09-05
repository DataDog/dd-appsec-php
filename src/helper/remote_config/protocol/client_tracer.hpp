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
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    client_tracer(std::string &runtime_id, std::string &tracer_version,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        std::string &service, std::string &env, std::string &app_version)
        : runtime_id_(runtime_id), tracer_version_(tracer_version),
          service_(service), env_(env), app_version_(app_version){};
    std::string get_runtime_id() { return runtime_id_; };
    std::string get_tracer_version() { return tracer_version_; };
    std::string get_service() { return service_; };
    std::string get_env() { return env_; };
    std::string get_app_version() { return app_version_; };
    bool operator==(client_tracer const &b) const
    {
        return runtime_id_ == b.runtime_id_ &&
               tracer_version_ == b.tracer_version_ && service_ == b.service_ &&
               env_ == b.env_ && app_version_ == b.app_version_;
    }

private:
    std::string runtime_id_;
    std::string tracer_version_;
    std::string service_;
    std::string env_;
    std::string app_version_;
};

} // namespace dds::remote_config::protocol
