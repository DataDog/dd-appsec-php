// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#pragma once

#include "exception.hpp"
#include <ddwaf.h>

namespace dds {

enum parameter_type : unsigned {
    invalid = DDWAF_OBJ_INVALID,
    int64 = DDWAF_OBJ_SIGNED,
    uint64 = DDWAF_OBJ_UNSIGNED,
    string = DDWAF_OBJ_STRING,
    map = DDWAF_OBJ_MAP,
    array = DDWAF_OBJ_ARRAY
};

class parameter_base : public ddwaf_object {
public:
    parameter_base() { ddwaf_object_invalid(this); }

    virtual ~parameter_base() = default;

    // Container size
    parameter_type type() const noexcept
    {
        return static_cast<parameter_type>(ddwaf_object::type);
    }
    [[nodiscard]] size_t size() const noexcept
    {
        if (!is_container()) {
            return 0;
        }
        return static_cast<size_t>(nbEntries);
    }
    [[nodiscard]] size_t length() const noexcept
    {
        if (!is_string()) {
            return 0;
        }
        return static_cast<size_t>(nbEntries);
    }
    [[nodiscard]] bool has_key() const noexcept
    {
        return parameterName != nullptr;
    }
    [[nodiscard]] std::string_view key() const noexcept
    {
        return {parameterName, parameterNameLength};
    }
    [[nodiscard]] bool is_map() const noexcept
    {
        return ddwaf_object::type == DDWAF_OBJ_MAP;
    }
    [[nodiscard]] bool is_container() const noexcept
    {
        return (ddwaf_object::type & (DDWAF_OBJ_MAP | DDWAF_OBJ_ARRAY)) != 0;
    }
    [[nodiscard]] bool is_string() const noexcept
    {
        return ddwaf_object::type == DDWAF_OBJ_STRING;
    }
    [[nodiscard]] bool is_unsigned() const noexcept
    {
        return ddwaf_object::type == DDWAF_OBJ_UNSIGNED;
    }
    [[nodiscard]] bool is_signed() const noexcept
    {
        return ddwaf_object::type == DDWAF_OBJ_SIGNED;
    }
    [[nodiscard]] bool is_valid() const noexcept
    {
        return ddwaf_object::type != DDWAF_OBJ_INVALID;
    }

    operator ddwaf_object *() noexcept { return this; }

    operator std::string_view() const
    {
        if (!is_string()) {
            throw bad_cast("parameter not a string");
        }
        return std::string_view(stringValue, nbEntries);
    }
    operator std::string() const
    {
        if (!is_string()) {
            throw bad_cast("parameter not a string");
        }
        return std::string(stringValue, nbEntries);
    }
    operator uint64_t() const
    {
        if (!is_unsigned()) {
            throw bad_cast("parameter not an uint64");
        }
        return uintValue;
    }
    operator int64_t() const
    {
        if (!is_signed()) {
            throw bad_cast("parameter not an int64");
        }
        return intValue;
    }

    [[nodiscard]] std::string debug_str() const noexcept;

    using length_type = decltype(nbEntries);
    static constexpr auto max_length{
        std::numeric_limits<length_type>::max() - 1};
};

} // namespace dds
