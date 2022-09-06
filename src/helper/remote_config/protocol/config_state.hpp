// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

class config_state {
public:
    config_state(
        const std::string &&id, int version, const std::string &&product)
        : id_(std::move(id)), version_(version), product_(std::move(product)){};
    [[nodiscard]] std::string get_id() const { return id_; };
    [[nodiscard]] int get_version() const { return version_; };
    [[nodiscard]] std::string get_product() const { return product_; };
    bool operator==(config_state const &b) const
    {
        return id_ == b.id_ && version_ == b.version_ && product_ == b.product_;
    }

private:
    std::string id_;
    int version_;
    std::string product_;
};

} // namespace dds::remote_config::protocol
