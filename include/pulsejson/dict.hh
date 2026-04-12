#pragma once

#include "detail/error.hh"
#include "detail/writer.hh"
#include <rapidjson/document.h>
#include <cstddef>
#include <exception>
#include <string>
#include <utility>

namespace pulsejson {

// ---------------------------------------------------------------------------
// dict_t<V> — reads/writes a JSON object as an associative container with
// std::string-compatible keys and mapped values handled by V.
// ---------------------------------------------------------------------------
template <typename V>
class dict_t
{
    V item_;

public:
    explicit dict_t(V item)
        : item_(std::move(item))
    {}

    dict_t(const dict_t&) = default;
    dict_t(dict_t&&) noexcept = default;

    template <typename Dict>
    void read(const rapidjson::Value& v, Dict& out) const
    {
        if (!v.IsObject())
            throw detail::expected("object", v);

        out.clear();
        for (auto it = v.MemberBegin(); it != v.MemberEnd(); ++it) {
            const std::string key{it->name.GetString(), it->name.GetStringLength()};
            typename Dict::mapped_type item{};
            try {
                item_.read(it->value, item);
            } catch (const std::exception& e) {
                throw detail::at_key(key, e);
            }
            out.emplace(key, std::move(item));
        }
    }

    template <typename Dict>
    void write(detail::IWriter& w, const Dict& dict) const
    {
        w.start_object();
        for (const auto& [key, item] : dict) {
            w.key_(key.c_str(), key.size());
            item_.write(w, item);
        }
        w.end_object();
    }
};

} // namespace pulsejson
