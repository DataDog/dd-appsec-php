// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "../../../json_helper.hpp"
#include "serializer.hpp"

namespace dds::remote_config::protocol::tuf {

const char *product_to_string(product product)
{
    switch (product) {
    case LIVE_DEBUGGING:
        return "LIVE_DEBUGGING";
    case ASM_DD:
        return "ASM_DD";
    case FEATURES:
        return "FEATURES";
    }
}

void serialize_client_tracer(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Value &client_field, client_tracer client_tracer)
{
    rapidjson::Value tracer_object(rapidjson::kObjectType);

    tracer_object.AddMember("language", "php", alloc);
    tracer_object.AddMember(
        "runtime_id", client_tracer.get_runtime_id(), alloc);
    tracer_object.AddMember(
        "tracer_version", client_tracer.get_tracer_version(), alloc);
    tracer_object.AddMember("service", client_tracer.get_service(), alloc);
    tracer_object.AddMember("env", client_tracer.get_env(), alloc);
    tracer_object.AddMember(
        "app_version", client_tracer.get_app_version(), alloc);

    client_field.AddMember("client_tracer", tracer_object, alloc);
}

void serialize_client(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Document &document, client client)
{
    rapidjson::Value client_object(rapidjson::kObjectType);

    client_object.AddMember("id", client.get_id(), alloc);

    rapidjson::Value products(rapidjson::kArrayType);
    for (const product p : client.get_products()) {
        products.PushBack(
            rapidjson::Value(product_to_string(p), alloc).Move(), alloc);
    }
    client_object.AddMember("products", products, alloc);

    serialize_client_tracer(alloc, client_object, client.get_tracer());

    document.AddMember("client", client_object, alloc);
}

dds_remote_config_result serialize(
    client_get_configs_request request, std::string &output)
{
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &alloc = document.GetAllocator();

    document.SetObject();

    serialize_client(alloc, document, request.get_client());

    dds::string_buffer buffer;
    rapidjson::Writer<decltype(buffer)> writer(buffer);

    // This has to be tested
    if (!document.Accept(writer)) {
        return dds_remote_config_result::ERROR;
    }

    output = std::move(buffer.get_string_ref());
    return dds_remote_config_result::SUCCESS;
}

} // namespace dds::remote_config::protocol::tuf
