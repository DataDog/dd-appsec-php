// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#pragma once

#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parameter_base.hpp"

namespace dds {

class parameter : public parameter_base {
public:
    parameter();
    explicit parameter(const ddwaf_object &arg);

    parameter(const parameter &) = delete;            // fault;
    parameter &operator=(const parameter &) = delete; // fault;

    parameter(parameter &&) noexcept;
    parameter &operator=(parameter &&) noexcept;

    // These will be freed by the WAF, if the parameters are not passed to the
    // WAF, expect a memory leak if "free" is not called.
    ~parameter() override { ddwaf_object_free(this); }

    static parameter map() noexcept;
    static parameter array() noexcept;
    static parameter uint64(uint64_t value) noexcept;
    static parameter int64(int64_t value) noexcept;
    static parameter string(const std::string &str) noexcept;
    static parameter string(std::string_view str) noexcept;

    bool add(parameter &&entry) noexcept;
    bool add(std::string_view name, parameter &&entry) noexcept;

    // The reference should be considered invalid after adding an element
    parameter &operator[](size_t index) const;
};

} // namespace dds
