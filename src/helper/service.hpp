// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.
#pragma once

#include "engine.hpp"
#include "engine_pool.hpp"
#include "exception.hpp"
#include "std_logging.hpp"
#include "subscriber/waf.hpp"
#include "utils.hpp"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <unordered_map>


namespace dds {

class service
{
public:
    using ptr = std::shared_ptr<service>;

    struct identifier {
        std::string service;
        std::string env;

        MSGPACK_DEFINE_MAP(service, env);

        bool operator==(const identifier &oth) const noexcept {
            return service == oth.service && env == oth.env;
        }

        friend auto &operator<<(std::ostream &os, const identifier &id) {
            return os << "{service=" << id.service
                      << ", env=" << id.env
                      << "}";
        }

        struct hash {
            std::size_t operator()(const identifier &id) const noexcept {
                return dds::hash(id.service, id.env);
            }
        };
    };

    explicit service(identifier id, std::shared_ptr<engine> &engine):
      id_(std::move(id)), engine_(std::move(engine)) {
          // The engine should always be valid
          // if (!engine_) throw;
      }


    static service::ptr from_settings(const identifier &id,
        const dds::client_settings &eng_settings,
        /*remote_config::settings &rc_settings,*/
        std::map<std::string_view, std::string> &meta,
        std::map<std::string_view, double> &metrics);

    [[nodiscard]] std::shared_ptr<engine> get_engine() const {
        // TODO make access atomic?
        return engine_;
    }

protected:
    identifier id_;
    std::shared_ptr<engine> engine_;
};

} // namespace dds 
