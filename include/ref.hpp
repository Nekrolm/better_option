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

#include <functional>
#include <type_traits>
#include <utility>

#include "void.hpp"

namespace better {

template <class T>
concept IsLvalueReference =
    std::is_reference_v<T> && std::is_lvalue_reference_v<T>;


// Custom reference type
// Reference types in C++ are not first class citizens
// they cannot be changed and cannot be referenced
template <class T>
struct Ref final {
    static_assert(!std::is_reference_v<T>);
    static_assert(!std::is_same_v<T, void>);

    using Const = Ref<std::add_const_t<T>>;

    // Reference is constructible only from l-values
    explicit Ref(T& x) : _ptr{&x} {}
    // Rvalues are banned!
    Ref(T&&) = delete;

    // Follow Rule of Zero.
    // There is nothing special for Reference type

    T& get() noexcept { return *_ptr; }
    // Propagate const for safety!
    std::add_const_t<T>& get() const noexcept { return std::as_const(*_ptr); }

    decltype(auto) operator*() noexcept { return get(); }

    // I don't have C++23 compiler with deducing this :(
    decltype(auto) operator*() const noexcept { return get(); }

    T* operator->() noexcept { return _ptr; }

    // Propagate const for safety!
    std::add_const_t<T>* operator->() const noexcept { return _ptr; }

    // Add support for implicit to reference conversion
    operator T&() noexcept { return get(); }
    // Add support for implicit to reference conversion
    operator std::add_const_t<T>&() const noexcept { return get(); }

    // Add support to const referene conversion
    operator Const() noexcept { return Const{get()}; }

    operator const Const&() const& noexcept {
        return reinterpret_cast<const Const&>(*this);
    }

    // Remove any implicit conversions to T that can be introduced by the
    // implicit conversion to const T&
    operator std::remove_const_t<T>() const = delete;
    operator std::remove_const_t<T>() = delete;

    template <class... Args>
    decltype(auto) operator()(Args&&... args) noexcept(
        noexcept(std::invoke(get(), std::forward<Args>(args)...)))
        requires std::is_invocable_v<T&, Args...>
    {
        return std::invoke(get(), std::forward<Args>(args)...);
    }

    template <class... Args>
    decltype(auto) operator()(Args&&... args) const
        noexcept(noexcept(std::invoke(get(), std::forward<Args>(args)...)))
        requires std::is_invocable_v<T&, Args...>
    {
        return std::invoke(get(), std::forward<Args>(args)...);
    }

    decltype(auto) operator()(Void) noexcept(noexcept(std::invoke(get())))
        requires std::is_invocable_v<T&>
    {
        return std::invoke(get());
    }
    decltype(auto) operator()(Void) const noexcept(noexcept(std::invoke(get())))
        requires std::is_invocable_v<std::add_const_t<T>&>
    {
        return std::invoke(get());
    }

    bool ref_equals(const Ref& other) const { return this->_ptr == other._ptr; }

  private:
    T* _ptr;
};

// Add deduction guides for Reference, for better syntax
template <class T>
Ref(T&) -> Ref<T>;
template <class T>
Ref(const T&) -> Ref<const T>;

template <class T>
constexpr bool IsRef = false;
template <class T>
constexpr bool IsRef<Ref<T>> = true;

template <class T>
auto make_const_ref_helper(const T& e) {
    if constexpr (IsRef<T>) {
        using ConstRef = typename T::Const;
        return Ref<const ConstRef>{e};
    } else {
        return Ref<const T>{e};
    }
}

template <class T>
using MakeConstRefType =
    decltype(make_const_ref_helper(std::declval<const T&>()));

} // namespace better