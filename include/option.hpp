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
#include "tags.hpp"
#include "void.hpp"

#include "invoke_with.hpp"

#include "storage/generic.hpp"
#include "storage/ref.hpp"

#include <bit>
#include <compare>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

#include <stdexcept>

namespace better {

template <class T>
struct Option : protected OptionStorage<T> {

    static_assert(!std::is_const_v<T>, "const cvalified types are not allowed");
    static_assert(!std::is_same_v<T, void>,
                  "built-in void type cannot be supported as a type parameter. "
                  "Use better::Void");
    static_assert(!std::is_reference_v<T>,
                  "built-in reference types cannot be supported as a type "
                  "parameter. Use better::Ref");
    static_assert(OptionStorageImpl<OptionStorage<T>, T>,
                  "OptionalStorage specialization doesn't satisfy "
                  "OptionalStorage interface");

  private:
    using Base = OptionStorage<T>;

  protected:
    using Base::unwrap_unsafe;

  public:
    using Base::is_some;

    bool is_none() const noexcept { return !this->is_some(); }

    explicit operator bool() const noexcept { return is_some(); }

    Option(NoneTag none) : Base(none) {}
    template <class... Args>
    Option(SomeTag some, Args&&... args)
        : Base(some, std::forward<Args>(args)...) {}

    Option& operator=(NoneTag) {
        this->take();
        return *this;
    }

    Option<T> take() {
        Option<T> tmp{None};
        this->swap(tmp);
        return tmp;
    }
    Option<T> insert(auto&&... args) {
        Option<T> tmp{Some, std::forward<decltype(args)>(args)...};
        this->swap(tmp);
        return tmp;
    }

    void swap(Option& other) { Base::swap(other); }

    const T& unwrap() const& {
        if (is_some()) {
            return this->unwrap_unsafe();
        } else {
            throw std::runtime_error("attempt to unwrap None");
        }
    }

    T& unwrap() & {
        if (is_some()) {
            return this->unwrap_unsafe();
        } else {
            throw std::runtime_error("attempt to unwrap None");
        }
    }

    T&& unwrap() && {
        if (is_some()) {
            return std::move(*this).unwrap_unsafe();
        } else {
            throw std::runtime_error("attempt to unwrap None");
        }
    }

    T unwrap_or_default() &&
        requires std::is_default_constructible_v<T> &&
                 std::is_move_constructible_v<T>
    {
        return is_some() ? std::move(*this).unwrap_unsafe() : T{};
    }

    T unwrap_or_default() const&
        requires std::is_default_constructible_v<T> &&
                 std::is_copy_constructible_v<T>
    {
        return is_some() ? this->unwrap_unsafe() : T{};
    }

    template <class U>
    T unwrap_or(U&& default_val) &&
        requires std::is_constructible_v<T, U&&> &&
                 std::is_move_constructible_v<T>
    {
        return is_some() ? std::move(*this).unwrap_unsafe()
                         : T{std::forward<U>(default_val)};
    }

    template <class U>
    T unwrap_or(U&& default_val) const&
        requires std::is_constructible_v<T, U&&> &&
                 std::is_copy_constructible_v<T>
    {
        return is_some() ? this->unwrap_unsafe()
                         : T{std::forward<U>(default_val)};
    }

    template <class F>
    T unwrap_or_else(F&& on_none) &&
        requires std::is_invocable_r_v<T, F&&>
    {
        return is_some() ? std::move(*this).unwrap_unsafe()
                         : std::invoke(std::forward<F>(on_none));
    }

    template <class F>
    T unwrap_or_else(F&& on_none) const&
        requires std::is_invocable_r_v<T, F&&> &&
                 std::is_copy_constructible_v<T>
    {
        return is_some() ? this->unwrap_unsafe()
                         : std::invoke(std::forward<F>(on_none));
    }

    auto as_ref() & {
        // Have to explicitly specify reference type otherwise
        // Ref { this->unwrap_unsafe() } may become a copy-construction
        // when T is Ref
        using RefT =
            Ref<std::remove_reference_t<decltype(this->unwrap_unsafe())>>;
        return is_some() ? Option<RefT>{Some, RefT{this->unwrap_unsafe()}}
                         : Option<RefT>{None};
    }

    auto as_ref() const& {
        // Have to explicitly specify reference type otherwise
        // Ref { this->unwrap_unsafe() } may become a copy-construction
        // when T is Ref
        using RefT =
            Ref<std::remove_reference_t<decltype(this->unwrap_unsafe())>>;
        return is_some() ? Option<RefT>{Some, RefT{this->unwrap_unsafe()}}
                         : Option<RefT>{None};
    }

    template <class F>
        requires IsInvocableWith<F, T>
    auto map(F&& f) && {
        using ResultT =
            decltype(invoke_with(std::forward<F>(f), std::declval<T>()));

        using OptT = Option<ResultT>;
        return is_some()
                   ? OptT{Some, invoke_with(std::forward<F>(f),
                                            std::move(*this).unwrap_unsafe())}
                   : OptT{None};
    }

    template <class F>
        requires IsInvocableWith<F, const T&>
    auto map(F&& f) const {
        using ResultT =
            decltype(invoke_with(std::forward<F>(f), std::declval<const T&>()));
            
        using OptT = Option<ResultT>;
        return is_some() ? OptT{Some, invoke_with(std::forward<F>(f),
                                                  this->unwrap_unsafe())}
                         : OptT{None};
    }

    template <class F>
    auto and_then(F&& f) &&
        requires IsInvocableWith<F, T> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward(f), std::declval<T>())),
                     NoneTag>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward(f), std::declval<T>()));
        return is_some() ? invoke_with(std::forward(f),
                                       std::move(*this).unwrap_unsafe())
                         : ResultOpt{None};
    }

    template <class F>
    auto and_then(F&& f) const&
        requires IsInvocableWith<F, const T&> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward(f),
                                          std::declval<const T&>())),
                     NoneTag>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward(f), std::declval<T>()));
        return is_some() ? invoke_with(std::forward(f), this->unwrap_unsafe())
                         : ResultOpt{None};
    }

    template <class F>
        requires std::is_invocable_r_v<Option<T>, F>
    Option<T> or_else(F&& f) && {
        return is_some() ? std::move(*this) : std::invoke(std::forward<F>(f));
    }

    template <class F>
        requires std::is_invocable_r_v<Option<T>, F>
    Option<T> or_else(F&& f) const& {
        return Option(*this).or_else(std::forward<F>(f));
    }

    auto operator<=>(const Option& other) const
        requires std::three_way_comparable<T>
    {
        using Ordering = std::compare_three_way_result_t<T>;
        const bool left_is_some = this->is_some();
        const bool right_is_some = this->is_some();
        if (left_is_some < right_is_some) {
            return Ordering::less;
        }
        if (left_is_some > right_is_some) {
            return Ordering::greater;
        }
        if (!left_is_some) {
            return Ordering::equivalent;
        } else {
            // both are Some
            return this->unwrap_unsafe() <=> other.unwrap_unsafe();
        }
    }

  private:
    explicit Option(Base&& base) noexcept(
        std::is_nothrow_move_constructible_v<Base>)
        : Base{std::move(base)} {}
};

template <class T>
Option(SomeTag, T) -> Option<T>;

static_assert(sizeof(Option<Void>) == sizeof(bool));

} // namespace better