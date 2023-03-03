// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <optional>
#include <string>

namespace dds::remote_config {

class http_api {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    http_api(std::string host, std::string port)
        : host_(std::move(host)), port_(std::move(port)){};

    http_api(const http_api &) = delete;
    http_api(http_api &&) = delete;

    http_api &operator=(const http_api &) = delete;
    http_api &operator=(http_api &&) = delete;

    virtual ~http_api() = default;

    virtual std::optional<std::string> get_info() const;
    virtual std::optional<std::string> get_configs(std::string &&request) const;

protected:
    std::string host_;
    std::string port_;
};

} // namespace dds::remote_config
