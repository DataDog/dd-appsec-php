// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#include <set>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "engine.hpp"
#include "engine_ruleset.hpp"
#include "engine_settings.hpp"
#include "exception.hpp"
#include "parameter_view.hpp"
#include "std_logging.hpp"
#include "subscriber/waf.hpp"

namespace dds {

void engine::subscribe(const subscriber::ptr &sub)
{
    for (const auto &addr : sub->get_subscriptions()) {
        auto it = subscriptions_.find(addr);
        if (it == subscriptions_.end()) {
            subscriptions_.emplace(
                addr, std::move(std::vector<subscriber::ptr>{sub}));
        } else {
            it->second.push_back(sub);
        }
    }
}

std::optional<engine::result> engine::context::publish(parameter &&param)
{
    // Once the parameter reaches this function, it is guaranteed to be
    // owned by the engine.
    prev_published_params_.push_back(std::move(param));

    parameter_view data(prev_published_params_.back());
    if (!data.is_map()) {
        throw invalid_object(".", "not a map");
    }

    std::set<subscriber::ptr> sub_set;
    for (const auto &entry : data) {
        auto key = entry.key();
        DD_STDLOG(DD_STDLOG_IG_DATA_PUSHED, key);
        auto it = subscriptions_.find(key);
        if (it == subscriptions_.end()) {
            continue;
        }
        for (auto &&sub : it->second) { sub_set.insert(sub); }
    }

    // Now that we have found the required subscriptions, find the current
    // context and pass the data.
    std::vector<std::string> event_data;
    std::unordered_set<std::string> event_actions;
    for (const auto &sub : sub_set) {
        auto it = listeners_.find(sub);
        if (it == listeners_.end()) {
            it = listeners_.emplace(sub, sub->get_listener()).first;
        }
        try {
            auto event = it->second->call(data);
            if (event) {
                event_data.insert(event_data.end(),
                    std::make_move_iterator(event->data.begin()),
                    std::make_move_iterator(event->data.end()));
                event_actions.merge(event->actions);
            }
        } catch (std::exception &e) {
            SPDLOG_ERROR("subscriber failed: {}", e.what());
        }
    }

    // Technically we could avoid processing any data if the limiter prevents
    // us from sending the event back to the extension.
    if ((event_data.empty() && event_actions.empty()) || !limiter_.allow()) {
        return std::nullopt;
    }

    dds::engine::result res{action_type::record, {}, std::move(event_data)};
    // Currently the only action the extension can perform is block
    if (!event_actions.empty()) {
        // The extension can only handle one action, so we pick the first one
        // available in the list of actions.
        for (const auto &action_str : event_actions) {
            auto it = actions_.find(action_str);
            if (it != actions_.end()) {
                res.type = it->second.type;
                res.parameters = it->second.parameters;
                break;
            }
        }
    }

    return res;
}

void engine::context::get_meta_and_metrics(
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics)
{
    for (const auto &[subscriber, listener] : listeners_) {
        listener->get_meta_and_metrics(meta, metrics);
    }
}

namespace {

template <typename T>
engine::action_map parse_actions(
    const T &doc, const engine::action_map &default_actions)
{
    engine::action_map actions = default_actions;
    /*    if (doc.GetType() != rapidjson::kArrayType) {*/
    /*// perhaps throw something?*/
    /*SPDLOG_ERROR(*/
    /*"unexpected WAF result type {}, expected array", doc.GetType());*/
    /*return actions;*/
    /*}*/

    /*for (auto &action : doc.GetArray()) {*/

    /*}*/

    return actions;
}
} // namespace

engine::ptr engine::from_settings(const dds::engine_settings &eng_settings,
    std::map<std::string_view, std::string> &meta,
    std::map<std::string_view, double> &metrics)

{
    auto &&rules_path = eng_settings.rules_file_or_default();
    auto ruleset = engine_ruleset::from_path(rules_path);
    auto actions =
        parse_actions(ruleset.get_document(), engine::default_actions);
    std::shared_ptr engine_ptr{
        engine::create(eng_settings.trace_rate_limit, std::move(actions))};

    try {
        SPDLOG_DEBUG("Will load WAF rules from {}", rules_path);
        // may throw std::exception
        const subscriber::ptr waf =
            waf::instance::from_settings(eng_settings, ruleset, meta, metrics);
        engine_ptr->subscribe(waf);
    } catch (...) {
        DD_STDLOG(DD_STDLOG_WAF_INIT_FAILED, rules_path);
        throw;
    }

    return engine_ptr;
}

// NOLINTNEXTLINE
const engine::action_map engine::default_actions = {
    {"block", {engine::action_type::block,
                  {{"status_code", "403"}, {"type", "auto"}}}},
};

} // namespace dds
