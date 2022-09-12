// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

class client_tracer {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    client_tracer(const std::string &runtime_id,
        const std::string &tracer_version,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        const std::string &service, const std::string &env,
        const std::string &app_version)
        : runtime_id_(std::move(runtime_id)),
          tracer_version_(std::move(tracer_version)),
          service_(std::move(service)), env_(std::move(env)),
          app_version_(std::move(app_version)){};
    [[nodiscard]] std::string get_runtime_id() const { return runtime_id_; };
    [[nodiscard]] std::string get_tracer_version() const
    {
        return tracer_version_;
    };
    [[nodiscard]] std::string get_service() const { return service_; };
    [[nodiscard]] std::string get_env() const { return env_; };
    [[nodiscard]] std::string get_app_version() const { return app_version_; };
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
