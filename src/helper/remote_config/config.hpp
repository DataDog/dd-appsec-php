// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace dds::remote_config {

struct config {
public:
    //@todo contents should be bytes
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    config(std::string &product, std::string &id, std::string &contents,
        std::map<std::string, std::string> &hashes, int version,
        std::string &path, int length)
        : product_(product), id_(id), contents_(contents), hashes_(hashes),
          version_(version), path_(path), length_(length){};
    bool operator==(config const &b) const
    {
        return product_ == b.product_ && id_ == b.id_ &&
               contents_ == b.contents_ && hashes_ == b.hashes_ &&
               version_ == b.version_ && path_ == b.path_ &&
               length_ == b.length_;
    }
    std::string get_id() { return id_; };
    [[nodiscard]] int get_version() const { return version_; };
    std::string get_product() { return product_; };
    std::string get_contents() { return contents_; };
    std::string get_path() { return path_; };
    [[nodiscard]] int get_length() const { return length_; };
    std::map<std::string, std::string> get_hashes() { return hashes_; };

private:
    std::string product_;
    std::string id_;
    std::string contents_;
    std::string path_;
    std::map<std::string, std::string> hashes_;
    int version_;
    int length_;
};

} // namespace dds::remote_config
