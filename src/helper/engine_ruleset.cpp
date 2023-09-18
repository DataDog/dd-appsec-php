// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <fstream>
#include <ios>
#include <rapidjson/error/en.h>

#include "engine_ruleset.hpp"
#include "exception.hpp"
#include "utils.hpp"

namespace dds {

engine_ruleset::engine_ruleset(std::string_view ruleset)
{
    rapidjson::ParseResult const result = doc_.Parse(ruleset.data());
    if ((result == nullptr) || !doc_.IsObject()) {
        throw parsing_error("invalid json rule");
    }
}

void engine_ruleset::add_default_processors()
{
    std::string processors_str =
        R"({"processors": [{"id": "processor-001", "generator": "extract_schema", "conditions": [{"operator": "equals", "parameters": {"inputs": [{"address": "waf.context.processor", "key_path": ["extract-schema"] } ], "type": "boolean", "value": true } } ], "parameters": {"mappings": [{"inputs": [{"address": "server.request.body"} ], "output": "_dd.appsec.s.req.body"}, {"inputs": [{"address": "server.request.headers.no_cookies"} ], "output": "_dd.appsec.s.req.headers"}, {"inputs": [{"address": "server.request.query"} ], "output": "_dd.appsec.s.req.query"}, {"inputs": [{"address": "server.request.path_params"} ], "output": "_dd.appsec.s.req.params"}, {"inputs": [{"address": "server.request.cookies"} ], "output": "_dd.appsec.s.req.cookies"}, {"inputs": [{"address": "server.response.headers.no_cookies"} ], "output": "_dd.appsec.s.res.headers"}, {"inputs": [{"address": "server.response.body"} ], "output": "_dd.appsec.s.res.body"} ] }, "evaluate": false, "output": true } ] })";
    rapidjson::Document::AllocatorType &alloc = doc_.GetAllocator();
    rapidjson::Document processors_doc(&alloc);
    rapidjson::ParseResult const processors =
        processors_doc.Parse(processors_str.data());
    if ((processors == nullptr) || !processors_doc.IsObject()) {
        return;
    }
    auto parsed_processors = processors_doc.FindMember("processors");
    if (parsed_processors == processors_doc.GetObject().MemberEnd()) {
        return;
    }
    doc_.AddMember("processors", parsed_processors->value, alloc);
}

engine_ruleset engine_ruleset::from_path(std::string_view path)
{
    auto ruleset = read_file(path);
    auto engine = engine_ruleset{ruleset};
    engine.add_default_processors();

    return engine;
}

} // namespace dds
