#include <limits>
#include <string>
#include <string_view>

#include "parameter_base.hpp"

namespace dds {

namespace {
// NOLINTNEXTLINE(misc-no-recursion,google-runtime-references)
void debug_str_helper(std::string &res, const parameter_base &p)
{
    if (p.parameterNameLength != 0U) {
        res += p.key();
        res += ": ";
    }
    switch (p.type()) {
    case parameter_type::invalid:
        res += "<invalid>";
        break;
    case parameter_type::int64:
        res += std::to_string(p.intValue);
        break;
    case parameter_type::uint64:
        res += std::to_string(p.uintValue);
        break;
    case parameter_type::string:
        res += '"';
        res += std::string_view{p.stringValue, p.nbEntries};
        res += '"';
        break;
    case parameter_type::array:
        res += '[';
        for (decltype(p.nbEntries) i = 0; i < p.nbEntries; i++) {
            debug_str_helper(res, static_cast<const parameter_base&>(p.array[i]));
            if (i != p.size() - 1) {
                res += ", ";
            }
        }
        res += ']';
        break;
    case parameter_type::map:
        res += '{';
        for (decltype(p.nbEntries) i = 0; i < p.nbEntries; i++) {
            debug_str_helper(res, static_cast<const parameter_base&>(p.array[i]));
            if (i != p.size() - 1) {
                res += ", ";
            }
        }
        res += '}';
        break;
    }
}
} // namespace

std::string parameter_base::debug_str() const noexcept
{
    std::string res;
    debug_str_helper(res, *this);
    return res;
}

}
