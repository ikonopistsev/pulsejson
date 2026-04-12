#pragma once

#include "detail/error.hh"
#include "detail/writer.hh"
#include <rapidjson/document.h>
#include <cstddef>
#include <utility>

namespace pulsejson {

// ---------------------------------------------------------------------------
// array_t<V> — reads/writes a JSON array using V as the per-element reader.
//
// V must provide:
//   void read(const rapidjson::Value& elem, ElemType& out) const;
//   void write(detail::IWriter& w, const ElemType& v) const;
//
// Container must support push_back() and clear() (std::vector, std::list, ...).
// ---------------------------------------------------------------------------
template <typename V>
class array_t
{
    V item_;

public:
    explicit array_t(V item) noexcept : item_(std::move(item)) {}

    array_t(const array_t&) = default;
    array_t(array_t&&) noexcept = default;

    template <typename Container>
    void read(const rapidjson::Value& v, Container& out) const
    {
        if (!v.IsArray())
            throw detail::expected("array", v);

        out.clear();
        std::size_t index = 0;
        for (const auto& elem : v.GetArray()) {
            typename Container::value_type item{};
            try {
                item_.read(elem, item);
            } catch (const std::exception& e) {
                throw detail::at_index(index, e);
            }
            out.push_back(std::move(item));
            ++index;
        }
    }

    template <typename Container>
    void write(detail::IWriter& w, const Container& cont) const
    {
        w.start_array();
        for (const auto& item : cont)
            item_.write(w, item);
        w.end_array();
    }
};

} // namespace pulsejson
