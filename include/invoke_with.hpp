/*
Copyright 2024 Dmitry Sviridkin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "ref.hpp"
#include "void.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace better {

template <class F, class... Args>
constexpr bool IsInvocableWith = std::is_invocable_v<F, Args...>;

template <class F, class V>
    requires std::is_same_v<Void, std::decay_t<V>>
constexpr bool IsInvocableWith<F, V> = std::is_invocable_v<F>;

namespace detail {

template <class R, class F, class... Args>
auto wrap_references(F&& f, Args&&... args) {
    if constexpr (std::is_reference_v<R>) {
        static_assert(std::is_lvalue_reference_v<R>,
                      "better::Ref doesn't support rvalue references");

        return Ref<std::remove_reference_t<R>>(
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
    } else {
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }
}

} // namespace detail

template <class F, class... Args>
decltype(auto) invoke_with(F&& f, Args&&... args)
    requires std::is_invocable_v<F, Args...>
{
    using R = std::invoke_result_t<F, Args...>;
    if constexpr (std::is_same_v<R, void>) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        return Void{};
    } else {
        return detail::wrap_references<R>(std::forward<F>(f),
                                          std::forward<Args>(args)...);
    }
}
template <class F>
decltype(auto) invoke_with(F&& f, Void)
    requires std::is_invocable_v<F>
{
    using R = std::invoke_result_t<F>;
    if constexpr (std::is_same_v<R, void>) {
        std::invoke(std::forward<F>(f));
        return Void{};
    } else {
        return detail::wrap_references<R>(std::forward<F>(f));
    }
}

} // namespace better