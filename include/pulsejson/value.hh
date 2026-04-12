#pragma once

#include "detail/error.hh"
#include "detail/scalars.hh"
#include "detail/writer.hh"
#include <rapidjson/document.h>
#include <utility>

namespace pulsejson {

struct field_options
{
    bool mandatory{};
};

inline constexpr field_options optional_field{};
inline constexpr field_options mandatory_field{true};

// ---------------------------------------------------------------------------
// value<C> — virtual base for a field-level reader/writer relative to C.
// Uses IWriter so that any rapidjson output stream (Writer, PrettyWriter, ...)
// can be used without changing the virtual dispatch hierarchy.
// ---------------------------------------------------------------------------
template <typename C>
struct value
{
    virtual ~value() = default;

    virtual void read(const rapidjson::Value& v, C& obj) const = 0;

    virtual void write(detail::IWriter& w, const C& obj) const = 0;

    virtual bool required() const noexcept { return false; }

    virtual void missing(C&) const {}
};

// ---------------------------------------------------------------------------
// value_t<T> — reads/writes a scalar (or nested object/array) of type T.
// Not a value<C>; embedded inside memptr_value_t to connect to a C field.
// ---------------------------------------------------------------------------
template <typename T>
class value_t
{
public:
    void read(const rapidjson::Value& v, T& obj) const
    {
        detail::rj_read<T>::from(v, obj);
    }

    void write(detail::IWriter& w, const T& obj) const
    {
        detail::iw_write<T>::to(w, obj);
    }
};

// ---------------------------------------------------------------------------
// memptr_value_t<T, C, Value> — value<C> that dispatches through T C::*ptr_
// ---------------------------------------------------------------------------
template <typename T, typename C, typename Value>
class memptr_value_t : public value<C>
{
    T C::*ptr_;
    Value value_;
    field_options options_;

public:
    memptr_value_t(T C::*ptr, Value val,
            field_options options = optional_field) noexcept
        : ptr_(ptr),
          value_(std::move(val)),
          options_(options)
    {}

    memptr_value_t(const memptr_value_t&) = default;
    memptr_value_t(memptr_value_t&&) noexcept = default;

    void read(const rapidjson::Value& v, C& obj) const override
    {
        value_.read(v, obj.*ptr_);
    }

    void write(detail::IWriter& w, const C& obj) const override
    {
        value_.write(w, obj.*ptr_);
    }

    bool required() const noexcept override
    {
        if (options_.mandatory)
            return true;

        if constexpr (requires { value_.required(); })
            return value_.required();
        else
            return false;
    }

    void missing(C& obj) const override
    {
        if (options_.mandatory)
            throw detail::missing_required_field();

        if constexpr (requires { value_.missing(obj.*ptr_); }) {
            value_.missing(obj.*ptr_);
        } else if (required()) {
            throw detail::missing_required_field();
        }
    }
};

// ---------------------------------------------------------------------------
// required_value_t<Value> — marks a field as mandatory while preserving the
// concrete reader/writer implementation.
// ---------------------------------------------------------------------------
template <typename Value>
class required_value_t
{
    Value value_;

public:
    explicit required_value_t(Value value)
        : value_(std::move(value))
    {}

    template <typename T>
    void read(const rapidjson::Value& v, T& obj) const
    {
        value_.read(v, obj);
    }

    template <typename T>
    void write(detail::IWriter& w, const T& obj) const
    {
        value_.write(w, obj);
    }

    bool required() const noexcept { return true; }

    template <typename T>
    void missing(T&) const
    {
        throw detail::missing_required_field();
    }
};

// ---------------------------------------------------------------------------
// dummy_value_t<T, C> — ignores input on read; emits null on write.
// Use for forward-compat: a key removed from the struct that may still
// appear in old config files.  During serialization it outputs `"key": null`
// so the JSON remains valid.
// ---------------------------------------------------------------------------
template <typename T, typename C>
class dummy_value_t : public value<C>
{
public:
    void read(const rapidjson::Value&, C&) const override {}
    void write(detail::IWriter& w, const C&) const override { w.null(); }
};

} // namespace pulsejson
