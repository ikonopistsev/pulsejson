#pragma once

#include "object.hh"
#include "array.hh"
#include "dict.hh"
#include "enum.hh"
#include "value.hh"
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// pulsejson — rapidjson-backed struct-descriptor JSON library.
//
// Declares C++ struct <-> JSON mappings using member pointers.
// The descriptor is defined once (typically as a const static) and reused
// for both parsing and serialization.
//
// Quick reference:
//
//   // 1. Define struct
//   struct service_cfg {
//       std::string name;
//       bool        enabled = false;
//       int         timeout_ms = 5000;
//   };
//
//   // 2. Declare descriptor
//   static const auto cfg_desc = pulsejson::make_object<service_cfg>(
//       pulsejson::must("name",       &service_cfg::name),
//       pulsejson::kv("enabled",      &service_cfg::enabled),
//       pulsejson::kv("timeout_ms",   &service_cfg::timeout_ms)
//   );
//
//   // 3. Generate
//   service_cfg cfg;
//   std::string compact = cfg_desc.to_string(cfg);
//   std::string pretty  = cfg_desc.to_pretty_string(cfg);
//
//   // 4. Stream into an existing rapidjson writer
//   rapidjson::PrettyWriter<rapidjson::StringBuffer> pw(buf);
//   cfg_desc.write_to(pw, cfg);
//
//   // 5. Parse config when needed
//   service_cfg parsed = cfg_desc.parse(json_string_view);
//
// Nested object:
//   pulsejson::pair("sub", pulsejson::object(&T::sub, sub_desc))
//
// Arrays of scalars (element type deduced from Container::value_type):
//   pulsejson::pair("ids", pulsejson::array(&T::ids))
//
// Arrays with explicit element reader (scalars or objects):
//   pulsejson::pair("items", pulsejson::array(&T::items, sub_desc))
//   pulsejson::pair("scores", pulsejson::array(&T::scores, pulsejson::value_t<double>{}))
//
// Unknown JSON keys are ignored by default (forward-compatibility), or rejected
// with object_options{unknown_policy::error}.
// ---------------------------------------------------------------------------

namespace pulsejson {

// ---------------------------------------------------------------------------
// make_object<C>(pair(...), ...) — construct an object_t<C> descriptor
// ---------------------------------------------------------------------------
template <typename C, typename... Pairs>
object_t<C> make_object(Pairs&&... pairs)
{
    return object_t<C>(std::forward<Pairs>(pairs)...);
}

template <typename C, typename... Pairs>
object_t<C> make_object(object_options options, Pairs&&... pairs)
{
    return object_t<C>(options, std::forward<Pairs>(pairs)...);
}

// ---------------------------------------------------------------------------
// pair("key", T C::*ptr) — scalar field
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto pair(std::string name, T C::*ptr)
{
    return std::make_pair(std::move(name),
        memptr_value_t<T, C, value_t<T>>(ptr, value_t<T>{}));
}

template <typename T, typename C>
auto pair(std::string name, T C::*ptr, field_options options)
{
    return std::make_pair(std::move(name),
        memptr_value_t<T, C, value_t<T>>(ptr, value_t<T>{}, options));
}

// ---------------------------------------------------------------------------
// pair("key", T C::*ptr, V value) — custom field reader/writer.
// Use for enum_string(), required(), custom object/array/dict descriptors, etc.
// ---------------------------------------------------------------------------
template <typename T, typename C, typename V>
auto pair(std::string name, T C::*ptr, V value)
{
    return std::make_pair(std::move(name),
        memptr_value_t<T, C, V>(ptr, std::move(value)));
}

template <typename T, typename C, typename V>
auto pair(std::string name, T C::*ptr, V value, field_options options)
{
    return std::make_pair(std::move(name),
        memptr_value_t<T, C, V>(ptr, std::move(value), options));
}

// ---------------------------------------------------------------------------
// pair("key", V value) — generic: wraps any pre-built value
//   (result of object() / array() helpers)
// ---------------------------------------------------------------------------
template <typename V>
std::pair<std::string, V> pair(std::string name, V value)
{
    return {std::move(name), std::move(value)};
}

// ---------------------------------------------------------------------------
// object(T C::*ptr, object_t<T> desc) — nested object field
//   pair("sub", pulsejson::object(&T::sub, sub_desc))
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto object(T C::*ptr, object_t<T> desc)
{
    return memptr_value_t<T, C, object_t<T>>(ptr, std::move(desc));
}

// ---------------------------------------------------------------------------
// array(Container C::*ptr) — array of scalars
//   Element type is deduced from Container::value_type.
//   pair("ids", pulsejson::array(&T::ids))
// ---------------------------------------------------------------------------
template <typename Container, typename C>
auto array(Container C::*ptr)
{
    using T = typename Container::value_type;
    return memptr_value_t<Container, C, array_t<value_t<T>>>(
        ptr, array_t<value_t<T>>{value_t<T>{}});
}

// ---------------------------------------------------------------------------
// array(Container C::*ptr, V item_reader) — array with explicit per-element
//   reader; use for object arrays or non-default scalar handling.
//   pair("items", pulsejson::array(&T::items, sub_desc))
// ---------------------------------------------------------------------------
template <typename Container, typename C, typename V>
auto array(Container C::*ptr, V item_reader)
{
    return memptr_value_t<Container, C, array_t<V>>(
        ptr, array_t<V>{std::move(item_reader)});
}

// ---------------------------------------------------------------------------
// dict(Map C::*ptr) — JSON object with string keys and scalar mapped values.
// ---------------------------------------------------------------------------
template <typename Dict, typename C>
auto dict(Dict C::*ptr)
{
    using T = typename Dict::mapped_type;
    return memptr_value_t<Dict, C, dict_t<value_t<T>>>(
        ptr, dict_t<value_t<T>>{value_t<T>{}});
}

// ---------------------------------------------------------------------------
// dict(Map C::*ptr, V item_reader) — JSON object with string keys and explicit
// mapped value reader; use for enum/object mapped values.
// ---------------------------------------------------------------------------
template <typename Dict, typename C, typename V>
auto dict(Dict C::*ptr, V item_reader)
{
    return memptr_value_t<Dict, C, dict_t<V>>(
        ptr, dict_t<V>{std::move(item_reader)});
}

// ---------------------------------------------------------------------------
// required("key", T C::*ptr[, V]) — compatibility spelling for mandatory field.
// Missing fields throw during read. Null values are valid only when V supports
// them, for example std::optional<T>.
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto required(std::string name, T C::*ptr)
{
    return pair(std::move(name), ptr, mandatory_field);
}

template <typename T, typename C, typename V>
auto required(std::string name, T C::*ptr, V value)
{
    return pair(std::move(name), ptr, std::move(value), mandatory_field);
}

// ---------------------------------------------------------------------------
// Short readable aliases for descriptor blocks.
// kv: scalar/custom value, obj: nested object, arr: JSON array.
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto kv(std::string name, T C::*ptr)
{
    return pair(std::move(name), ptr);
}

template <typename T, typename C, typename V>
auto kv(std::string name, T C::*ptr, V value)
{
    return pair(std::move(name), ptr, std::move(value));
}

template <typename T, typename C>
auto obj(std::string name, T C::*ptr, object_t<T> desc)
{
    return pair(std::move(name), object(ptr, std::move(desc)));
}

template <typename Container, typename C>
auto arr(std::string name, Container C::*ptr)
{
    return pair(std::move(name), array(ptr));
}

template <typename Container, typename C, typename V>
auto arr(std::string name, Container C::*ptr, V item_reader)
{
    return pair(std::move(name), array(ptr, std::move(item_reader)));
}

template <typename Dict, typename C>
auto dict(std::string name, Dict C::*ptr)
{
    return pair(std::move(name), dict(ptr));
}

template <typename Dict, typename C, typename V>
auto dict(std::string name, Dict C::*ptr, V item_reader)
{
    return pair(std::move(name), dict(ptr, std::move(item_reader)));
}

template <typename T, typename C>
auto required_obj(std::string name, T C::*ptr, object_t<T> desc)
{
    return required(std::move(name), ptr, std::move(desc));
}

template <typename Container, typename C>
auto required_arr(std::string name, Container C::*ptr)
{
    return required(std::move(name), ptr,
        array_t<value_t<typename Container::value_type>>{
            value_t<typename Container::value_type>{}});
}

template <typename Container, typename C, typename V>
auto required_arr(std::string name, Container C::*ptr, V item_reader)
{
    return required(std::move(name), ptr,
        array_t<V>{std::move(item_reader)});
}

template <typename Dict, typename C>
auto required_dict(std::string name, Dict C::*ptr)
{
    return required(std::move(name), ptr,
        dict_t<value_t<typename Dict::mapped_type>>{
            value_t<typename Dict::mapped_type>{}});
}

template <typename Dict, typename C, typename V>
auto required_dict(std::string name, Dict C::*ptr, V item_reader)
{
    return required(std::move(name), ptr,
        dict_t<V>{std::move(item_reader)});
}

// ---------------------------------------------------------------------------
// must* — preferred mandatory-field spelling.
// This is the same behavior as required*, but reads closer to kv/obj/arr.
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto must(std::string name, T C::*ptr)
{
    return required(std::move(name), ptr);
}

template <typename T, typename C, typename V>
auto must(std::string name, T C::*ptr, V value)
{
    return required(std::move(name), ptr, std::move(value));
}

template <typename T, typename C>
auto must_obj(std::string name, T C::*ptr, object_t<T> desc)
{
    return required_obj(std::move(name), ptr, std::move(desc));
}

template <typename Container, typename C>
auto must_arr(std::string name, Container C::*ptr)
{
    return required_arr(std::move(name), ptr);
}

template <typename Container, typename C, typename V>
auto must_arr(std::string name, Container C::*ptr, V item_reader)
{
    return required_arr(std::move(name), ptr, std::move(item_reader));
}

template <typename Dict, typename C>
auto must_dict(std::string name, Dict C::*ptr)
{
    return required_dict(std::move(name), ptr);
}

template <typename Dict, typename C, typename V>
auto must_dict(std::string name, Dict C::*ptr, V item_reader)
{
    return required_dict(std::move(name), ptr, std::move(item_reader));
}

// ---------------------------------------------------------------------------
// dummy<T, C>("key") — field consumed from JSON but discarded
//   Use for forward-compatibility when a field is removed from the struct
//   but may still appear in old config files.
// ---------------------------------------------------------------------------
template <typename T, typename C>
auto dummy(std::string name)
{
    return std::make_pair(std::move(name), dummy_value_t<T, C>{});
}

} // namespace pulsejson
