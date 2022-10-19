// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#pragma once

#include "utils.hpp"
#include <algorithm>
#include <cstdint>
#include <msgpack.hpp>
#include <ostream>
#include <string>

namespace dds::remote_config {

/* client_settings are currently the same for the whole client session.
 * If this changes in the future, it will make sense to create a separation
 * between 1) settings used for creating the engine and 2) settings used
 * after, possibly when creating the subscriber listeners on every request
 */
struct settings {
    static constexpr uint32_t default_poll_interval{1000};
    static constexpr uint64_t default_max_payload_size{4096};
    // Remote config settings
    bool enabled{false};
    bool integrity_check_enabled{true};
    std::string host;
    std::string port;
    std::string url; // optional
    std::uint32_t initial_poll_interval = default_poll_interval;
    std::uint64_t max_payload_size = default_max_payload_size;

    // these two are specified in RCTE1
    std::string targets_key;
    std::string targets_key_id;

    MSGPACK_DEFINE_MAP(enabled, integrity_check_enabled, host, port, url,
        initial_poll_interval, max_payload_size, targets_key, targets_key_id);

    bool operator==(const settings &oth) const noexcept
    {
        return enabled == oth.enabled &&
               integrity_check_enabled == oth.integrity_check_enabled &&
               url == oth.url &&
               initial_poll_interval == oth.initial_poll_interval &&
               max_payload_size == oth.max_payload_size &&
               targets_key == oth.targets_key &&
               targets_key_id == oth.targets_key_id;
    }

    friend auto &operator<<(std::ostream &os, const settings &c)
    {
        return os << "{enabled=" << c.enabled
                  << ", integrity_check_enabled="
                  << c.integrity_check_enabled << ", url=" << c.url
                  << ", initial_poll_interval=" << c.initial_poll_interval
                  << ", max_payload_size=" << c.max_payload_size
                  << ", targets_key=" << c.targets_key
                  << ", targets_key_id=" << c.targets_key_id << "}";
    }

    struct settings_hash {
        std::size_t operator()(const settings &s) const noexcept
        {
            return hash(s.enabled, s.integrity_check_enabled, s.url,
                s.initial_poll_interval, s.max_payload_size, s.targets_key,
                s.targets_key_id);
        }
    };
};
} // namespace dds::remote_config
