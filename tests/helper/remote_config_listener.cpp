// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "base64.h"
#include "common.hpp"
#include "remote_config/listener.hpp"
#include "remote_config/product.hpp"

namespace dds {

remote_config::config get_config(const std::string &non_encoded_content)
{
    std::string encoded_content = base64_encode(non_encoded_content);
    return {"some product", "some id", encoded_content, "some path", {}, 123,
        321,
        remote_config::protocol::config_state_applied_state::UNACKNOWLEDGED,
        ""};
}

remote_config::config get_config_with_status(std::string status)
{
    return get_config("{\"asm\":{\"enabled\":\"" + status + "\"}}");
}

remote_config::config get_enabled_config()
{
    return get_config_with_status("true");
}

remote_config::config get_disabled_config()
{
    return get_config_with_status("false");
}

TEST(RemoteConfigAsmFeaturesListener, ByDefaultListenerIsNotActive)
{
    remote_config::asm_features_listener listener;

    EXPECT_FALSE(listener.is_active());
}

TEST(RemoteConfigAsmFeaturesListener, ListenerGetActiveWhenConfigSaysSoOnUpdate)
{
    remote_config::asm_features_listener listener;

    try {
        listener.on_update(get_enabled_config());
    } catch (remote_config::error_applying_config &error) {
        std::cout << error.what() << std::endl;
    }

    EXPECT_TRUE(listener.is_active());
}

TEST(RemoteConfigAsmFeaturesListener,
    ListenerGetDeactivedWhenConfigSaysSoOnUpdate)
{
    remote_config::asm_features_listener listener;

    try {
        listener.on_update(get_disabled_config());
    } catch (remote_config::error_applying_config &error) {
        std::cout << error.what() << std::endl;
    }

    EXPECT_FALSE(listener.is_active());
}
} // namespace dds