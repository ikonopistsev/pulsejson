#pragma once

#include <tuple>

namespace pulsejson::detail {

template <typename F, typename... Args>
void for_each(std::tuple<Args...>& t, F&& f)
{
    std::apply([&](auto&... elems) { (f(elems), ...); }, t);
}

template <typename F, typename... Args>
void for_each(const std::tuple<Args...>& t, F&& f)
{
    std::apply([&](const auto&... elems) { (f(elems), ...); }, t);
}

} // namespace pulsejson::detail
