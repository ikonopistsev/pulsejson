#pragma once

#include "value.hh"
#include "detail/error.hh"
#include "detail/prototype.hh"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace pulsejson {

enum class unknown_policy
{
    ignore,
    error
};

struct object_options
{
    unknown_policy unknown = unknown_policy::ignore;
};

inline constexpr object_options ignore_unknown{};
inline constexpr object_options error_unknown{unknown_policy::error};

// ---------------------------------------------------------------------------
// object_t<C> — descriptor for a JSON object that maps to struct C.
//
// Constructed via make_object<C>(pair(...), pair(...), ...).
//
// Reading:
//   read(string_view json, C& obj)      — parse JSON string and fill obj
//   read(const rapidjson::Value&, C&)   — fill from pre-parsed DOM node
//
// Writing:
//   to_string(obj)                      — compact JSON string
//   to_pretty_string(obj)               — indented JSON string
//   write_to(rapid_writer, obj)         — stream into any rapidjson writer
//   write(IWriter&, obj)                — stream into abstract writer
//
// Unknown JSON keys are ignored by default and can be rejected with
// object_options{unknown_policy::error}.
// ---------------------------------------------------------------------------
template <typename C>
class object_t
{
    using proto        = detail::prototype<C>;
    using sorted_pairs = typename proto::sorted_pairs;

    object_options          options_;
    typename proto::pointer prototype_;
    sorted_pairs            sorted_pairs_;

public:
    template <typename... Pairs>
    explicit object_t(Pairs&&... pairs)
        : object_t(object_options{}, std::forward<Pairs>(pairs)...)
    {}

    template <typename... Pairs>
    explicit object_t(object_options options, Pairs&&... pairs)
        : options_(options),
          prototype_(proto::create(std::forward<Pairs>(pairs)...))
    {
        prototype_->sort(sorted_pairs_);
    }

    object_t(const object_t& other)
        : options_(other.options_),
          prototype_(other.prototype_->clone())
    {
        prototype_->sort(sorted_pairs_);
    }

    object_t(object_t&&) noexcept = default;

    // ----------------------------------------------------------------
    // read — from a DOM node (nested use inside memptr_value_t)
    // ----------------------------------------------------------------
    void read(const rapidjson::Value& v, C& obj) const
    {
        if (!v.IsObject())
            throw detail::expected("object", v);

        std::set<std::string> seen;
        for (auto it = v.MemberBegin(); it != v.MemberEnd(); ++it) {
            const std::string key{it->name.GetString(), it->name.GetStringLength()};
            auto found = sorted_pairs_.find(key);
            if (found == sorted_pairs_.end()) {
                if (options_.unknown == unknown_policy::error)
                    throw std::runtime_error{"pulsejson: unknown field '" +
                        key + "'"};
                continue;
            }

            seen.insert(key);
            try {
                found->second->read(it->value, obj);
            } catch (const std::exception& e) {
                throw detail::at_key(key, e);
            }
        }

        for (const auto& [key, field] : sorted_pairs_) {
            if (seen.find(key) != seen.end())
                continue;

            try {
                field->missing(obj);
            } catch (const std::exception& e) {
                throw detail::at_key(key, e);
            }
        }
    }

    // ----------------------------------------------------------------
    // read — parse a JSON string and fill obj (primary entry point)
    // ----------------------------------------------------------------
    void read(std::string_view json, C& obj) const
    {
        rapidjson::Document doc;
        doc.Parse(json.data(), json.size());
        if (doc.HasParseError())
            throw std::runtime_error{"pulsejson: parse error at offset " +
                std::to_string(doc.GetErrorOffset()) + ": " +
                rapidjson::GetParseError_En(doc.GetParseError())};
        read(static_cast<const rapidjson::Value&>(doc), obj);
    }

    // ----------------------------------------------------------------
    // parse — construct C{} first, then read into it.
    // This is the preferred config-file entry point: missing optional fields
    // naturally keep the struct's default constructor values.
    // ----------------------------------------------------------------
    [[nodiscard]] C parse(const rapidjson::Value& v) const
    {
        C obj{};
        read(v, obj);
        return obj;
    }

    [[nodiscard]] C parse(std::string_view json) const
    {
        C obj{};
        read(json, obj);
        return obj;
    }

    // ----------------------------------------------------------------
    // write — abstract writer (used by memptr_value_t for nesting)
    // ----------------------------------------------------------------
    void write(detail::IWriter& w, const C& obj) const
    {
        w.start_object();
        prototype_->write(w, obj);
        w.end_object();
    }

    // ----------------------------------------------------------------
    // write_to — stream directly into any rapidjson writer
    //   write_to(rapidjson::Writer<StringBuffer>, obj)
    //   write_to(rapidjson::PrettyWriter<StringBuffer>, obj)
    // ----------------------------------------------------------------
    template <typename RapidWriter>
    void write_to(RapidWriter& rw, const C& obj) const
    {
        auto adapter = detail::make_writer(rw);
        write(adapter, obj);
    }

    // ----------------------------------------------------------------
    // to_string — serialize to compact JSON string
    // ----------------------------------------------------------------
    std::string to_string(const C& obj) const
    {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> rw(buf);
        write_to(rw, obj);
        return buf.GetString();
    }

    // ----------------------------------------------------------------
    // to_pretty_string — serialize to indented JSON string
    // ----------------------------------------------------------------
    std::string to_pretty_string(const C& obj) const
    {
        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> rw(buf);
        write_to(rw, obj);
        return buf.GetString();
    }
};

} // namespace pulsejson
