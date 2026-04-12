#pragma once

#include "tuple.hh"
#include "writer.hh"
#include <map>
#include <memory>
#include <tuple>
#include <string>
#include <utility>

namespace pulsejson {

template <typename C>
struct value; // forward-declared in value.hh

namespace detail {

template <typename C>
struct prototype
{
    using value_pointer = const value<C>*;
    using sorted_pairs  = std::map<std::string, value_pointer, std::less<>>;

    // ------------------------------------------------------------------
    // sort_next — populates sorted_pairs_ from the pairs tuple
    // ------------------------------------------------------------------
    struct sort_next
    {
        sorted_pairs& sp_;

        explicit sort_next(sorted_pairs& sp) noexcept : sp_(sp) {}

        template <typename Value>
        void operator()(const std::pair<std::string, Value>& p)
        {
            sp_.insert({p.first, static_cast<value_pointer>(&p.second)});
        }
    };

    // ------------------------------------------------------------------
    // write_next — emits key + value for each pair during serialization
    // ------------------------------------------------------------------
    struct write_next
    {
        IWriter& w_;
        const C& obj_;

        write_next(IWriter& w, const C& obj) noexcept : w_(w), obj_(obj) {}

        template <typename Value>
        void operator()(const std::pair<std::string, Value>& p)
        {
            w_.key_(p.first.c_str(), p.first.size());
            p.second.write(w_, obj_);
        }
    };

    // ------------------------------------------------------------------
    // pairs_holder_base / pairs_holder
    // ------------------------------------------------------------------
    struct pairs_holder_base
    {
        virtual ~pairs_holder_base() = default;

        virtual std::unique_ptr<pairs_holder_base> clone() const = 0;
        virtual void sort(sorted_pairs& sp)                      = 0;
        virtual void write(IWriter& w, const C& obj) const       = 0;
    };

    using pointer = std::unique_ptr<pairs_holder_base>;

    template <typename... Pairs>
    class pairs_holder : public pairs_holder_base
    {
        std::tuple<Pairs...> pairs_;

        explicit pairs_holder(const std::tuple<Pairs...>& pairs)
            : pairs_(pairs)
        {}

    public:
        explicit pairs_holder(Pairs&&... pairs)
            : pairs_(std::move(pairs)...)
        {}

        pointer clone() const override
        {
            return pointer{new pairs_holder(pairs_)};
        }

        void sort(sorted_pairs& sp) override
        {
            for_each(pairs_, sort_next(sp));
        }

        void write(IWriter& w, const C& obj) const override
        {
            for_each(pairs_, write_next(w, obj));
        }
    };

    template <typename... Pairs>
    static pointer create(Pairs&&... pairs)
    {
        return pointer{new pairs_holder<Pairs...>(std::move(pairs)...)};
    }
};

} // namespace detail
} // namespace pulsejson
