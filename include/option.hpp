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

#include <bit>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

#include <stdexcept>

#include "ref.hpp"
#include "void.hpp"

namespace better {

struct NoneTag {};
struct SomeTag {};

constexpr inline NoneTag None;
constexpr inline SomeTag Some;

template <class F, class... Args>
constexpr bool IsInvocableWith = std::is_invocable_v<F, Args...>;

template <class F>
constexpr bool IsInvocableWith<F, Void> = std::is_invocable_v<F>;

template <class F, class... Args>
decltype(auto) invoke_with(F&& f, Args&&... args)
    requires std::is_invocable_v<F, Args...>
{
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

template <class F>
decltype(auto) invoke_with(F&& f, Void)
    requires std::is_invocable_v<F>
{
    return std::invoke(std::forward<F>(f));
}

// for easy swap, use extra struct
template <class T> struct RawStorage {
    alignas(T) std::byte data[sizeof(T)];

    char* get_bytes() noexcept { return reinterpret_cast<char*>(data); }

    T* get_raw() noexcept { return std::launder(reinterpret_cast<T*>(data)); }

    const T* get_raw() const noexcept {
        return std::launder(reinterpret_cast<const T*>(data));
    }
};

// Mandatory interface for OptionStorage:
// swap(Other)
// is_some()
// unwrap_unsafe()
//
// and two constructors:
// OptionStorage(NoneTag)
// OptionStorage(SomeTag, ...)

// Specializations must implement them all
template <class T> struct OptionStorage {

    bool is_some() const noexcept { return _initialized; }

    void swap(OptionStorage<T>& other) noexcept(
        std::is_trivially_move_constructible_v<T> ||
        std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_move_constructible_v<T>) {
            std::swap(this->_storage, other._storage);
            std::swap(this->_initialized, other._initialized);
            return;
        } else {
            if (other._initialized && this->_initialized) {
                std::swap(this->unwrap_unsafe(), other.unwrap_unsafe());
                return;
            }
            if (other._initialized) {
                this->construct(std::move(other).unwrap_unsafe());
                other.reset();
                return;
            }
            if (this->_initialized) {
                other.construct(std::move(this->unwrap_unsafe()));
                this->reset();
                return;
            }
        }
        // both None, do nothing
    }

    T& unwrap_unsafe() & noexcept { return *_storage.get_raw(); }
    T&& unwrap_unsafe() && noexcept { return std::move(*_storage.get_raw()); }
    const T& unwrap_unsafe() const& noexcept { return *_storage.get_raw(); }

    OptionStorage(NoneTag) noexcept : OptionStorage() {}

    template <class... Args>
    OptionStorage(SomeTag, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        requires std::is_constructible_v<T, Args...>
    {
        this->construct(std::forward<Args>(args)...);
    }

    OptionStorage(const OptionStorage&) noexcept
        requires(std::is_trivially_copy_constructible_v<T>)
    = default;
    OptionStorage(OptionStorage&& other) noexcept
        requires(std::is_trivially_move_constructible_v<T>)
    {
        this->_storage = other._storage;
        this->_initialized = other._initialized;
        other.reset();
    }

    OptionStorage& operator=(const OptionStorage&) noexcept
        requires(std::is_trivially_copy_assignable_v<T>)
    = default;
    OptionStorage& operator=(OptionStorage&& other) noexcept
        requires(std::is_trivially_move_assignable_v<T>)
    {
        if (this != std::addressof(other)) {
            this->_storage = other.storage;
            this->_initialized = other.initialized;
            other.reset();
        }
        return *this;
    }

    ~OptionStorage()
        requires(std::is_trivially_destructible_v<T>)
    = default;

    OptionStorage(const OptionStorage& other) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        requires(!std::is_trivially_copy_constructible_v<T>)
    {
        if (other.is_some()) {
            this->construct(other.unwrap_unsafe());
        }
    }

    // moves and resets other storage!
    OptionStorage(OptionStorage&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>)
    {
        if (other.is_some()) {
            this->construct(std::move(other).unwrap_unsafe());
            other.reset();
        }
    }

    OptionStorage& operator=(const OptionStorage& other) noexcept(
        std::is_nothrow_copy_constructible_v<T> &&
        noexcept(this->swap(std::declval<OptionStorage&>())))
        requires(!std::is_trivially_copy_assignable_v<T>)
    {
        if (this != std::addressof(other)) {
            OptionStorage tmp(other);
            this->swap(tmp);
        }

        return *this;
    }

    // moves and resets other storage!
    OptionStorage& operator=(OptionStorage&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        noexcept(this->swap(std::declval<OptionStorage&>())))
        requires(!std::is_trivially_move_assignable_v<T>)
    {
        if (this != std::addressof(other)) {
            OptionStorage tmp(std::move(other));
            this->swap(tmp);
        }

        return *this;
    }

    ~OptionStorage() noexcept(std::is_nothrow_destructible_v<T>)
        requires(!std::is_trivially_destructible_v<T>)
    {
        reset();
    }

  private:
    OptionStorage() noexcept = default;

    template <class... Args>
    void construct(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        requires std::is_constructible_v<T, Args...>
    {
        new (this->_storage.get_bytes()) T(std::forward<Args>(args)...);
        this->_initialized = true;
    }

    void reset() noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            if (_initialized) {
                _storage.get_raw()->~T();
            }
        }
        _initialized = false;
    }

    RawStorage<T> _storage;
    bool _initialized = false;
};

// Void specialization this one is easy
template <> struct OptionStorage<Void> : protected Void {

    const Void& unwrap_unsafe() const& { return *this; }
    Void& unwrap_unsafe() & { return *this; }
    Void&& unwrap_unsafe() && { return std::move(*this); }

    bool is_some() const noexcept { return _is_some; }
    void swap(OptionStorage& other) noexcept {
        std::swap(this->_is_some, other._is_some);
    }

    OptionStorage(NoneTag) noexcept : OptionStorage(false) {}
    OptionStorage(SomeTag, auto&&...) noexcept : OptionStorage(true) {}

    OptionStorage(const OptionStorage&) = default;
    OptionStorage(OptionStorage&& other) noexcept
        : OptionStorage(std::exchange(other._is_some, false)) {}

    OptionStorage& operator=(const OptionStorage&) = default;
    OptionStorage& operator=(OptionStorage&& other) noexcept {
        OptionStorage tmp{std::move(other)};
        this->swap(tmp);
        return *this;
    }

    ~OptionStorage() = default;

  private:
    explicit OptionStorage(bool initialized) noexcept : _is_some{initialized} {}

    bool _is_some = false;
};

template <class T> struct OptionStorage<Ref<T>> {

    bool is_some() const noexcept { return storage.raw != nullptr; }

    Ref<T>& unwrap_unsafe() & noexcept { return storage.ref; }

    const Ref<typename Ref<T>::Const>& unwrap_unsafe() const& noexcept {
        return storage.const_ref;
    }

    Ref<T>&& unwrap_unsafe() && noexcept {
        return std::move(this->storage.ref);
    }

    void swap(OptionStorage& other) noexcept {
        std::swap(this->storage, other.storage);
    }

    OptionStorage(NoneTag) noexcept
        : OptionStorage(RawStorage{.raw = nullptr}) {}
    OptionStorage(SomeTag, Ref<T> ref) noexcept
        : OptionStorage(RawStorage{ref}) {}

    // Explicitly delete constructors from Raw references
    // Clients must explicitly Use Ref to avoid confusion
    // and to not introduce dangling references
    OptionStorage(SomeTag, T&) = delete;
    OptionStorage(SomeTag, T&&) = delete;

    OptionStorage(const OptionStorage&) = default;
    OptionStorage(OptionStorage&& other) noexcept
        : OptionStorage(other.take()) {}

    OptionStorage& operator=(const OptionStorage&) = default;
    OptionStorage& operator=(OptionStorage&& other) noexcept {
        OptionStorage tmp{std::move(other)};
        this->swap(tmp);
        return *this;
    }

    ~OptionStorage() = default;

  private:
    union RawStorage {
        Ref<T> ref;
        Ref<typename Ref<T>::Const> const_ref;
        T* raw;
    } storage;

    explicit OptionStorage(RawStorage raw) noexcept : storage{raw} {
        static_assert(sizeof(RawStorage) == sizeof(T*));
    }
};

template <class T> struct Option : private OptionStorage<T> {
    static_assert(!std::is_const_v<T>, "const cvalified types are not allowed");
    static_assert(!std::is_same_v<T, void>,
                  "built-in void type cannot be supported as a type parameter. "
                  "Use better::Void");
    static_assert(!std::is_reference_v<T>,
                  "built-in reference types cannot be supported as a type "
                  "parameter. Use better::Ref");

  private:
    using Base = OptionStorage<T>;

  public:
    // using Base::Base;
    using Base::is_some;

    bool is_none() const noexcept { return !this->is_some(); }

    explicit operator bool() const noexcept { return is_some(); }

    Option(NoneTag none) : Base(none) {}
    template <class... Args>
    Option(SomeTag some, Args&&... args)
        : Base(some, std::forward<Args>(args)...) {}

    ~Option() = default;
    Option(const Option&) = default;
    Option& operator=(const Option&) = default;
    // Option<

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

    T unwrap() && {
        if (is_some()) {
            return take().unwrap_unsafe();
        } else {
            throw std::runtime_error("attempt to unwrap None");
        }
    }

    T unwrap() const
        requires std::is_trivially_copyable_v<T>
    {
        return Option(*this).unwrap();
    }

    T unwrap_or_default() &&
        requires std::is_default_constructible_v<T>
    {
        return is_some() ? take().unwrap_unsafe() : T{};
    }

    T unwrap_or_default() const
        requires std::is_default_constructible_v<T> &&
                 std::is_trivially_copyable_v<T>
    {
        return Option(*this).unwrap_or_default();
    }

    template <class U>
    T unwrap_or(U&& default_val) &&
        requires std::is_constructible_v<T, U&&>
    {
        return is_some() ? take().unwrap_unsafe()
                         : T{std::forward<U>(default_val)};
    }

    template <class U>
    T unwrap_or(U&& default_val) const
        requires std::is_constructible_v<T, U&&> &&
                 std::is_trivially_copyable_v<T>
    {
        return Option(*this).unwrap_or(std::forward<U>(default_val));
    }

    template <class F>
    T unwrap_or_else(F&& on_none) &&
        requires std::is_invocable_r_v<T, F&&>
    {
        return is_some() ? take().unwrap_unsafe()
                         : std::invoke(std::forward<F>(on_none));
    }

    template <class F>
    T unwrap_or_else(F&& on_none) &&
        requires std::is_invocable_r_v<T, F&&> &&
                 std::is_trivially_copyable_v<T>
    {
        return Option(*this).unwrap_or_else(std::forward<F>(on_none));
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
        if constexpr (std::is_same_v<ResultT, void>) {
            if (is_some()) {
                invoke_with(std::forward<F>(f), this->take().unwrap_unsafe());
                return Option<Void>{Some};
            } else {
                return Option<Void>{None};
            }
        } else if constexpr (std::is_reference_v<ResultT>) {
            static_assert(std::is_lvalue_reference_v<ResultT>,
                          "better::Option doesn't support rvalue references");
            using OptT = Option<Ref<std::remove_reference_t<ResultT>>>;
            return is_some()
                       ? OptT{Some,
                              Ref(invoke_with(std::forward<F>(f),
                                              this->take().unwrap_unsafe()))}
                       : OptT{None};
        } else {
            using OptT = Option<ResultT>;
            return is_some()
                       ? OptT{Some, invoke_with(std::forward<F>(f),
                                                this->take().unwrap_unsafe())}
                       : OptT{None};
        }
    }

    template <class F>
        requires IsInvocableWith<F, T> && std::is_trivially_copyable_v<T>
    auto map(F&& f) const {
        return Option(*this).map(std::forward<F>(f));
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
        return is_some()
                   ? invoke_with(std::forward(f), this->take().unwrap_unsafe())
                   : ResultOpt{None};
    }

    template <class F>
    auto and_then(F&& f) const
        requires IsInvocableWith<F, T> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward(f), std::declval<T>())),
                     NoneTag> &&
                 std::is_trivially_copyable_v<T>
    {
        return Option(*this).and_then(std::forward<F>(f));
    }

    template <class F>
        requires std::is_invocable_r_v<Option<T>, F>
    Option<T> or_else(F&& f) && {
        return is_some() ? this->take() : std::invoke(std::forward<F>(f));
    }

    template <class F>
        requires std::is_invocable_r_v<Option<T>, F> &&
                 std::is_trivially_copyable_v<T>
    Option<T> or_else(F&& f) const {
        return Option(*this).or_else(std::forward<F>(f));
    }

  private:
    explicit Option(Base&& base) noexcept(
        std::is_nothrow_move_constructible_v<Base>)
        : Base{std::move(base)} {}
};

template <class T> Option(SomeTag, T) -> Option<T>;

} // namespace better