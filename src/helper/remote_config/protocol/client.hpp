// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <string>

namespace dds::remote_config::protocol {

class Client {
public:
    Client(std::string id) : id(id){};
    std::string getId() { return this->id; };

private:
    std::string id;
};

} // namespace dds::remote_config::protocol
