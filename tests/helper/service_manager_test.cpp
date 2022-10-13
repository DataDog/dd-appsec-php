// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "client_settings.hpp"
#include "common.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <service_manager.hpp>
#include <tags.hpp>

namespace algo = boost::algorithm;

namespace dds {

TEST(ServiceManagerTest, DefaultRulesFile)
{
    auto path = client_settings::default_rules_file();
    EXPECT_TRUE(algo::ends_with(path, "/etc/dd-appsec/recommended.json"));
}

struct service_manager_exp : public service_manager {
    auto &get_cache() { return cache_; }
};

TEST(ServiceManagerTest, LoadRulesOK)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    service::identifier sid{"service", "env"};
    service_manager_exp manager;
    auto fn = create_sample_rules_ok();
    auto service = manager.create_service(sid, {fn, 42}, meta, metrics);
    EXPECT_EQ(manager.get_cache().size(), 1);
    EXPECT_EQ(metrics[tag::event_rules_loaded], 2);

    // loading again should take from the cache
    auto service2 = manager.create_service(sid, {fn, 42}, meta, metrics);
    EXPECT_EQ(manager.get_cache().size(), 1);

    // destroying the services should expire the cache ptr
    auto cache_it = manager.get_cache().begin();
    ASSERT_NE(cache_it, manager.get_cache().end());

    std::weak_ptr<dds::service> weak_ptr = cache_it->second;
    ASSERT_FALSE(weak_ptr.expired());
    service.reset();
    ASSERT_FALSE(weak_ptr.expired());
    service2.reset();
    // the last one should be kept by the manager
    ASSERT_FALSE(weak_ptr.expired());

    // loading another service should cleanup the cache
    service::identifier sid2{"service2", "env"};
    auto service3 = manager.create_service(sid2, {fn, 42}, meta, metrics);
    ASSERT_TRUE(weak_ptr.expired());
    EXPECT_EQ(manager.get_cache().size(), 1);

    // another service identifier should result in another service
    auto service4 = manager.create_service(sid, {fn, 42}, meta, metrics);
    EXPECT_EQ(manager.get_cache().size(), 2);
}

TEST(ServiceManagerTest, LoadRulesFileNotFound)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    service_manager_exp manager;
    EXPECT_THROW(
        {
            manager.create_service(
                {"s", "e"}, {"/file/that/does/not/exist", 42}, meta, metrics);
        },
        std::runtime_error);
}
TEST(ServiceManagerTest, BadRulesFile)
{
    std::map<std::string_view, std::string> meta;
    std::map<std::string_view, double> metrics;

    service_manager_exp manager;
    EXPECT_THROW(
        {
            manager.create_service({"s","e"}, {"/dev/null", 42}, meta, metrics);
        },
        dds::parsing_error);
}
} // namespace dds
