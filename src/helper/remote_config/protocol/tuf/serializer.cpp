// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "../../../json_helper.hpp"
#include "../cached_target_files.hpp"
#include "serializer.hpp"

namespace dds::remote_config::protocol {

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

void serialize_config_states(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Value &client_field,
    const std::vector<config_state> &config_states)
{
    rapidjson::Value config_states_object(rapidjson::kArrayType);

    for (auto config_state : config_states) {
        rapidjson::Value config_state_object(rapidjson::kObjectType);
        config_state_object.AddMember("id", config_state.get_id(), alloc);
        config_state_object.AddMember(
            "version", config_state.get_version(), alloc);
        config_state_object.AddMember(
            "product", config_state.get_product(), alloc);
        config_states_object.PushBack(config_state_object, alloc);
    }

    client_field.AddMember("config_states", config_states_object, alloc);
}

void serialize_client_state(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Value &client_field, client_state client_state)
{
    rapidjson::Value client_state_object(rapidjson::kObjectType);

    client_state_object.AddMember(
        "targets_version", client_state.get_targets_version(), alloc);
    client_state_object.AddMember("root_version", 1, alloc);
    client_state_object.AddMember(
        "has_error", client_state.get_has_error(), alloc);
    client_state_object.AddMember("error", client_state.get_error(), alloc);
    client_state_object.AddMember(
        "backend_client_state", client_state.get_backend_client_state(), alloc);

    serialize_config_states(
        alloc, client_state_object, client_state.get_config_states());

    client_field.AddMember("state", client_state_object, alloc);
}

void serialize_client(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Document &document, client client)
{
    rapidjson::Value client_object(rapidjson::kObjectType);

    client_object.AddMember("id", client.get_id(), alloc);
    client_object.AddMember("is_tracer", true, alloc);

    rapidjson::Value products(rapidjson::kArrayType);
    for (std::string &product_str : client.get_products()) {
        products.PushBack(rapidjson::Value(product_str, alloc).Move(), alloc);
    }
    client_object.AddMember("products", products, alloc);

    serialize_client_tracer(alloc, client_object, client.get_tracer());
    serialize_client_state(alloc, client_object, client.get_client_state());

    document.AddMember("client", client_object, alloc);
}

void serialize_cached_target_files_hashes(
    rapidjson::Document::AllocatorType &alloc, rapidjson::Value &parent,
    const std::vector<cached_target_files_hash> &cached_target_files_hash_list)
{
    rapidjson::Value cached_target_files_array(rapidjson::kArrayType);

    for (cached_target_files_hash ctfh : cached_target_files_hash_list) {
        rapidjson::Value cached_target_file_hash_object(rapidjson::kObjectType);
        cached_target_file_hash_object.AddMember(
            "algorithm", ctfh.get_algorithm(), alloc);
        cached_target_file_hash_object.AddMember(
            "hash", ctfh.get_hash(), alloc);
        cached_target_files_array.PushBack(
            cached_target_file_hash_object, alloc);
    }

    parent.AddMember("hashes", cached_target_files_array, alloc);
}

void serialize_cached_target_files(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Document &document,
    const std::vector<cached_target_files> &cached_target_files_list)
{
    rapidjson::Value cached_target_files_array(rapidjson::kArrayType);

    for (cached_target_files ctf : cached_target_files_list) {
        rapidjson::Value cached_target_file_object(rapidjson::kObjectType);
        cached_target_file_object.AddMember("path", ctf.get_path(), alloc);
        cached_target_file_object.AddMember("length", ctf.get_length(), alloc);
        serialize_cached_target_files_hashes(
            alloc, cached_target_file_object, ctf.get_hashes());
        cached_target_files_array.PushBack(cached_target_file_object, alloc);
    }

    document.AddMember("cached_target_files", cached_target_files_array, alloc);
}

std::optional<std::string> serialize(get_configs_request request)
{
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &alloc = document.GetAllocator();

    document.SetObject();

    serialize_client(alloc, document, request.get_client());
    serialize_cached_target_files(
        alloc, document, request.get_cached_target_files());

    dds::string_buffer buffer;
    rapidjson::Writer<decltype(buffer)> writer(buffer);

    // This has to be tested
    if (!document.Accept(writer)) {
        return std::nullopt;
    }

    return buffer.get_string_ref();
}

} // namespace dds::remote_config::protocol
