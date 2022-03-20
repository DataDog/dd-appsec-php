// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include "common.hpp"
#include <parameter.hpp>
#include <exception.hpp>

const std::string waf_rule =
    R"({"version":"1.0","events":[{"id":1,"tags":{"type":"flow1"},"conditions":[{"operation":"match_regex","parameters":{"inputs":["arg1"],"regex":"^string.*"}},{"operation":"match_regex","parameters":{"inputs":["arg2"],"regex":".*"}}],"action":"record"}]})";

namespace dds {

TEST(ParameterTest, EmptyConstructor)
{
    parameter p;
    EXPECT_EQ(p.type(), parameter_type::invalid);
    EXPECT_FALSE(p.is_valid());
}

TEST(ParameterTest, UintMaxConstructor)
{
    uint64_t value = std::numeric_limits<uint64_t>::max();
    parameter p= parameter::uint64(value);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    std::stringstream ss;
    ss << value;
    EXPECT_TRUE(p.stringValue == ss.str());
}

TEST(ParameterTest, UintMinConstructor)
{
    uint64_t value = std::numeric_limits<uint64_t>::min();
    parameter p = parameter::uint64(value);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    std::stringstream ss;
    ss << value;
    EXPECT_TRUE(p.stringValue == ss.str());
}

TEST(ParameterTest, IntMaxConstructor)
{
    int64_t value = std::numeric_limits<int64_t>::max();
    parameter p = parameter::int64(value);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    std::stringstream ss;
    ss << value;
    EXPECT_TRUE(p.stringValue == ss.str());
}

TEST(ParameterTest, IntMinConstructor)
{
    int64_t value = std::numeric_limits<int64_t>::min();
    parameter p = parameter::int64(value);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    std::stringstream ss;
    ss << value;
    EXPECT_TRUE(p.stringValue == ss.str());
}

TEST(ParameterTest, StringConstructor)
{
    std::string value("thisisastring");
    parameter p = parameter::string(value);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    EXPECT_TRUE(p.stringValue == value);
}

TEST(ParameterTest, StringViewConstructor)
{
    parameter p = parameter::string("thisisastring"sv);
    EXPECT_EQ(p.type(), parameter_type::string);
    EXPECT_NE(p.stringValue, nullptr);
    EXPECT_THROW(p[0], std::out_of_range);

    EXPECT_TRUE(p.stringValue == "thisisastring"sv);
}

TEST(ParameterTest, MoveConstructor)
{
    parameter p = parameter::string("thisisastring"sv);
    parameter pcopy(std::move(p));

    EXPECT_EQ(pcopy.type(), parameter_type::string);
    EXPECT_STREQ(pcopy.stringValue, "thisisastring");

    EXPECT_FALSE(p.is_valid());
}

TEST(ParameterTest, ObjectConstructor)
{
    ddwaf_object pw{};
    pw.parameterName = strdup("param");
    pw.parameterNameLength = sizeof("param") - 1;
    pw.stringValue = strdup("stringValue");
    pw.ddwaf_object::type = DDWAF_OBJ_STRING;

    parameter p(pw);
    EXPECT_EQ(p.parameterName, pw.parameterName);
    EXPECT_EQ(p.parameterNameLength, pw.parameterNameLength);
    EXPECT_EQ(p.stringValue, pw.stringValue);
    EXPECT_EQ(p.type(), pw.type);
}

TEST(ParameterTest, Map)
{
    parameter p = parameter::map();
    EXPECT_EQ(p.type(), parameter_type::map);
    EXPECT_EQ(p.size(), 0);

    EXPECT_TRUE(p.add("key0", parameter::string("value"sv)));
    EXPECT_STREQ(p[0].key().data(), "key0");
    EXPECT_EQ(p.size(), 1);

    EXPECT_TRUE(p.add("key1", parameter::string("value"sv)));
    EXPECT_STREQ(p[1].key().data(), "key1");
    EXPECT_EQ(p.size(), 2);

    EXPECT_TRUE(p.add("key2", parameter::string("value"sv)));
    EXPECT_STREQ(p[2].key().data(), "key2");
    EXPECT_EQ(p.size(), 3);

    EXPECT_TRUE(p.add("key3", parameter::string("value"sv)));
    EXPECT_STREQ(p[3].key().data(), "key3");
    EXPECT_EQ(p.size(), 4);

    auto v = parameter::string("value"sv);
    EXPECT_TRUE(p.add("key4", std::move(v)));
    EXPECT_STREQ(p[4].key().data(), "key4");
    EXPECT_EQ(p.size(), 5);

    EXPECT_FALSE(p.add(parameter::string("value"sv)));

    v = parameter::string("value"sv);
    EXPECT_FALSE(p.add(std::move(v)));

    EXPECT_THROW(std::string_view(p).data(), bad_cast);
}

TEST(ParameterTest, Array)
{
    parameter p = parameter::array();
    EXPECT_EQ(p.type(), parameter_type::array);
    EXPECT_EQ(p.size(), 0);

    EXPECT_TRUE(p.add(parameter::string("value"sv)));
    EXPECT_EQ(p.size(), 1);

    EXPECT_TRUE(p.add(parameter::string("value"sv)));
    EXPECT_EQ(p.size(), 2);

    EXPECT_TRUE(p.add(parameter::string("value"sv)));
    EXPECT_EQ(p.size(), 3);

    EXPECT_TRUE(p.add(parameter::string("value"sv)));
    EXPECT_EQ(p.size(), 4);

    auto v = parameter::string("value"sv);
    EXPECT_TRUE(p.add(std::move(v)));
    EXPECT_EQ(p.size(), 5);

    EXPECT_FALSE(p.add("key", parameter::string("value"sv)));

    v = parameter::string("value"sv);
    EXPECT_FALSE(p.add("key", std::move(v)));

    EXPECT_THROW(std::string_view(p).data(), bad_cast);
}

} // namespace dds
