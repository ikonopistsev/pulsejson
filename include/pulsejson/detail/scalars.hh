#pragma once

#include "error.hh"
#include "writer.hh"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <rapidjson/document.h>

namespace pulsejson::detail {

// ---------------------------------------------------------------------------
// rj_read<T> — reads a scalar from a rapidjson DOM Value.
// ---------------------------------------------------------------------------

template <typename T>
struct rj_read
{
    static_assert(sizeof(T) == 0,
        "pulsejson: no rj_read specialization for this type");
};

template <>
struct rj_read<bool>
{
    static void from(const rapidjson::Value& v, bool& out)
    {
        if (!v.IsBool())
            throw expected("bool", v);
        out = v.GetBool();
    }
};

template <>
struct rj_read<int>
{
    static void from(const rapidjson::Value& v, int& out)
    {
        if (!v.IsInt())
            throw expected("int", v);
        out = v.GetInt();
    }
};

template <>
struct rj_read<unsigned>
{
    static void from(const rapidjson::Value& v, unsigned& out)
    {
        if (!v.IsUint())
            throw expected("unsigned integer", v);
        out = v.GetUint();
    }
};

template <>
struct rj_read<std::int64_t>
{
    static void from(const rapidjson::Value& v, std::int64_t& out)
    {
        if (!v.IsInt64())
            throw expected("int64", v);
        out = v.GetInt64();
    }
};

template <>
struct rj_read<std::uint64_t>
{
    static void from(const rapidjson::Value& v, std::uint64_t& out)
    {
        if (!v.IsUint64())
            throw expected("uint64", v);
        out = v.GetUint64();
    }
};

template <>
struct rj_read<double>
{
    static void from(const rapidjson::Value& v, double& out)
    {
        if (!v.IsNumber())
            throw expected("number", v);
        out = v.GetDouble();
    }
};

template <>
struct rj_read<float>
{
    static void from(const rapidjson::Value& v, float& out)
    {
        if (!v.IsNumber())
            throw expected("number", v);
        out = static_cast<float>(v.GetDouble());
    }
};

template <>
struct rj_read<std::string>
{
    static void from(const rapidjson::Value& v, std::string& out)
    {
        if (!v.IsString())
            throw expected("string", v);
        out.assign(v.GetString(), v.GetStringLength());
    }
};

template <typename T>
struct rj_read<std::optional<T>>
{
    static void from(const rapidjson::Value& v, std::optional<T>& out)
    {
        if (v.IsNull()) {
            out.reset();
            return;
        }

        T value{};
        rj_read<T>::from(v, value);
        out = std::move(value);
    }
};

template <>
struct rj_read<std::chrono::milliseconds>
{
    static void from(const rapidjson::Value& v, std::chrono::milliseconds& out)
    {
        if (!v.IsInt64())
            throw expected("integer milliseconds", v);
        out = std::chrono::milliseconds{v.GetInt64()};
    }
};

// ---------------------------------------------------------------------------
// iw_write<T> — writes a scalar through IWriter.
// ---------------------------------------------------------------------------

template <typename T>
struct iw_write
{
    static_assert(sizeof(T) == 0,
        "pulsejson: no iw_write specialization for this type");
};

template <>
struct iw_write<bool>
{
    static void to(IWriter& w, const bool v)   { w.bool_(v); }
};

template <>
struct iw_write<int>
{
    static void to(IWriter& w, const int v)    { w.int_(v); }
};

template <>
struct iw_write<unsigned>
{
    static void to(IWriter& w, const unsigned v) { w.uint_(v); }
};

template <>
struct iw_write<std::int64_t>
{
    static void to(IWriter& w, const std::int64_t v) { w.int64_(v); }
};

template <>
struct iw_write<std::uint64_t>
{
    static void to(IWriter& w, const std::uint64_t v) { w.uint64_(v); }
};

template <>
struct iw_write<double>
{
    static void to(IWriter& w, const double v) { w.double_(v); }
};

template <>
struct iw_write<float>
{
    static void to(IWriter& w, const float v)
    { w.double_(static_cast<double>(v)); }
};

template <>
struct iw_write<std::string>
{
    static void to(IWriter& w, const std::string& v)
    { w.string_(v.c_str(), v.size()); }
};

template <typename T>
struct iw_write<std::optional<T>>
{
    static void to(IWriter& w, const std::optional<T>& v)
    {
        if (!v) {
            w.null();
            return;
        }

        iw_write<T>::to(w, *v);
    }
};

template <>
struct iw_write<std::chrono::milliseconds>
{
    static void to(IWriter& w, const std::chrono::milliseconds& v)
    {
        w.int64_(v.count());
    }
};

} // namespace pulsejson::detail
