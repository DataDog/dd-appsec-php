#pragma once

namespace dds {

namespace tag {
constexpr const char *event_rules_loaded  = "_dd.appsec.event_rules.loaded";
constexpr const char *event_rules_failed  = "_dd.appsec.event_rules.error_count";
constexpr const char *event_rules_errors  = "_dd.appsec.event_rules.errors";
constexpr const char *event_rules_version = "_dd.appsec.event_rules.version";

constexpr const char *waf_version = "_dd.appsec.waf.version";
constexpr const char *waf_duration = "_dd.appsec.waf.version";
}

}
