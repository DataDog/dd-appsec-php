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

void serialize_client(rapidjson::Document::AllocatorType &alloc,
    rapidjson::Document &document, Client client)
{
    rapidjson::Value client_object(rapidjson::kObjectType);

    client_object.AddMember("id", client.getId(), alloc);
    document.AddMember("client", client_object, alloc);
}

dds_remote_config_result serialize(
    ClientGetConfigsRequest request, std::string &output)
{
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &alloc = document.GetAllocator();

    document.SetObject();

    serialize_client(alloc, document, request.getClient());

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
