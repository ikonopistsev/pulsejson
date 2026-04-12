#pragma once

#include "detail/error.hh"
#include "detail/writer.hh"
#include <rapidjson/document.h>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pulsejson {

template <typename T>
class enum_string_t
{
    std::vector<std::pair<std::string, T>> values_;

public:
    explicit enum_string_t(std::initializer_list<std::pair<std::string, T>> values)
        : values_(values)
    {}

    void read(const rapidjson::Value& v, T& out) const
    {
        if (!v.IsString())
            throw detail::expected("string enum", v);

        const std::string_view name{v.GetString(), v.GetStringLength()};
        for (const auto& [candidate, value] : values_) {
            if (candidate == name) {
                out = value;
                return;
            }
        }

        throw std::runtime_error{"pulsejson: unknown enum literal '" +
            std::string{name} + "'"};
    }

    void write(detail::IWriter& w, const T& obj) const
    {
        for (const auto& [name, value] : values_) {
            if (value == obj) {
                w.string_(name.c_str(), name.size());
                return;
            }
        }

        throw std::runtime_error{"pulsejson: enum value has no JSON literal"};
    }
};

template <typename T>
enum_string_t<T> enum_string(
    std::initializer_list<std::pair<std::string, T>> values)
{
    return enum_string_t<T>{values};
}

} // namespace pulsejson
