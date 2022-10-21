// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include <remote_config/client.hpp>
#include <service.hpp>

namespace dds {

namespace mock {
class client : public remote_config::client {
public:
    client(const service_identifier &sid)
        : remote_config::client(nullptr, sid, {})
    {}
    ~client() = default;
    MOCK_METHOD0(poll, bool());
};
} // namespace mock

TEST(ServiceTest, DefaultService)
{
    service_identifier sid{
        "service", "env", "tracer_version", "app_version", "runtime_id"};
    std::shared_ptr<engine> engine{engine::create()};

    auto client = std::make_unique<mock::client>(sid);
    EXPECT_CALL(*client, poll).WillOnce(Return(true));

    service svc{sid, engine, std::move(client), 1s};

    // wait a little bit
    std::this_thread::sleep_for(20ms);
}

} // namespace dds
