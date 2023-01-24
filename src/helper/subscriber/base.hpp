// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "../engine_settings.hpp"
#include "../parameter.hpp"
#include "../parameter_view.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace dds {

class subscriber {
public:
    using ptr = std::shared_ptr<subscriber>;

    struct event {
        event() = default;

        event(std::vector<std::string> &&data_,
            std::unordered_set<std::string> &&actions_)
            : data(std::move(data_)), actions(std::move(actions_))
        {}
        event(const event &) = default;
        event(event &&) = default;
        event &operator=(const event &) = default;
        event &operator=(event &&) = default;
        ~event() = default;

        std::vector<std::string> data;
        std::unordered_set<std::string> actions;
    };

    class listener {
    public:
        using ptr = std::shared_ptr<listener>;

        listener() = default;
        listener(const listener &) = default;
        listener &operator=(const listener &) = delete;
        listener(listener &&) = default;
        listener &operator=(listener &&) = delete;

        virtual ~listener() = default;
        // NOLINTNEXTLINE(google-runtime-references)
        virtual std::optional<event> call(parameter_view &data) = 0;

        // NOLINTNEXTLINE(google-runtime-references)
        virtual void get_meta_and_metrics(
            std::map<std::string_view, std::string> &meta,
            std::map<std::string_view, double> &metrics) = 0;
    };

    subscriber() = default;
    virtual ~subscriber() = default;

    subscriber(const subscriber &) = delete;
    subscriber &operator=(const subscriber &) = delete;
    subscriber(subscriber &&) = delete;
    subscriber &operator=(subscriber &&) = delete;

    virtual std::vector<std::string_view> get_subscriptions() = 0;
    virtual listener::ptr get_listener() = 0;
};

} // namespace dds
