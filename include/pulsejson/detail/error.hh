#pragma once

#include <rapidjson/document.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pulsejson::detail {

inline const char* json_type_name(const rapidjson::Value& v) noexcept
{
    switch (v.GetType()) {
    case rapidjson::kNullType:
        return "null";
    case rapidjson::kFalseType:
    case rapidjson::kTrueType:
        return "bool";
    case rapidjson::kObjectType:
        return "object";
    case rapidjson::kArrayType:
        return "array";
    case rapidjson::kStringType:
        return "string";
    case rapidjson::kNumberType:
        return "number";
    }

    return "unknown";
}

inline std::string strip_error_prefix(std::string_view message)
{
    constexpr std::string_view prefix{"pulsejson: "};
    if (message.starts_with(prefix))
        message.remove_prefix(prefix.size());

    return std::string{message};
}

inline std::runtime_error expected(std::string_view expected_type,
    const rapidjson::Value& v)
{
    return std::runtime_error{"pulsejson: expected " +
        std::string{expected_type} + ", got " + json_type_name(v)};
}

inline std::runtime_error missing_required_field()
{
    return std::runtime_error{"pulsejson: missing required field"};
}

inline std::runtime_error at_key(std::string_view key, const std::exception& e)
{
    return std::runtime_error{"pulsejson: at '" + std::string{key} +
        "': " + strip_error_prefix(e.what())};
}

inline std::runtime_error at_index(const std::size_t index,
    const std::exception& e)
{
    return std::runtime_error{"pulsejson: at [" + std::to_string(index) +
        "]: " + strip_error_prefix(e.what())};
}

} // namespace pulsejson::detail
