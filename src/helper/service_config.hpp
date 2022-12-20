// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "remote_config/protocol/config_state.hpp"
#include <atomic>
#include <optional>

namespace dds {

enum class enable_asm_status : unsigned { NOT_SET = 0, ENABLED, DISABLED };

struct service_config {
    void enable_asm() { asm_enabled = true; }
    void disable_asm() { asm_enabled = false; }
    void unset_asm() { asm_enabled = std::nullopt; }
    std::optional<bool> get_asm_enabled_status() { return asm_enabled; }

protected:
    std::atomic<std::optional<bool>> asm_enabled = {std::nullopt};
};

} // namespace dds
