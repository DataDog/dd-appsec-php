#pragma once

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parameter.hpp"
#include "parameter_base.hpp"

namespace dds {

class parameter_view : public parameter_base {
public:
    class iterator {
    public:
        iterator(const parameter_view &pv, size_t index = 0):
            current_(pv.array+index),end_(pv.array+pv.nbEntries) {}

        bool operator!=(const iterator &rhs) const noexcept {
            return current_ != rhs.current_;
        }

        const parameter_view& operator*() const {
            return static_cast<const parameter_view&>(*current_);
        }

        iterator & operator++() noexcept {
            if (current_ != end_) {
                current_++;
            }
            return *this;
        }

    protected:
        const ddwaf_object *current_{nullptr};
        const ddwaf_object *end_{nullptr};
    };

    parameter_view() : parameter_base() {}
    explicit parameter_view(const ddwaf_object &arg) {
        *((ddwaf_object *)this) = arg;
    }

    explicit parameter_view(const parameter &arg) {
        *((ddwaf_object *)this) = (const ddwaf_object&)arg;
    }

    parameter_view(const parameter_view &) = default;
    parameter_view &operator=(const parameter_view &) = default;

    parameter_view(parameter_view&&) = delete;
    parameter_view operator=(parameter_view&&) = delete;

    ~parameter_view() override = default;

    iterator begin() const
    {
        if (!is_container()) {
            throw;
        }
        return iterator(*this);
    }
    iterator end() const
    {
        if (!is_container()) {
            throw;
        }
        return iterator(*this, size());
    }

    parameter_view operator[](size_t index) const
    {
        if (!is_container() || index >= size()) {
            throw std::out_of_range("index(" + std::to_string(index) +
                    ") out of range(" + std::to_string(size()) + ")");
        }
        return static_cast<parameter_view>(ddwaf_object::array[index]);
    }


};

} // namespace dds
