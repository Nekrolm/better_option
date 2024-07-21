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

#include "generic_option.hpp"
#include "raw.hpp"

#include "../ref.hpp"
#include "../tags.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace better {

// Mandatory interface for ResultStorage:
// swap(Other)
// is_ok()
// unwrap_unsafe()
// unwrap_err_unsafe()
//
// and two constructors:
// ResultStorage(ErrTag, ...)
// ResultStorage(OkTag, ...)
// Specializations must implement them all
template <class Storage, class T, class E>
concept ResultStorageImpl =
    requires(Storage& mut_storage, const Storage& const_storage) {
        { const_storage.is_ok() } -> std::same_as<bool>;
        { mut_storage.swap(mut_storage) };

        { mut_storage.unwrap_unsafe() } -> IsLvalueReference;
        { const_storage.unwrap_unsafe() } -> IsLvalueReference;
        { mut_storage.unwrap_err_unsafe() } -> IsLvalueReference;
        { const_storage.unwrap_err_unsafe() } -> IsLvalueReference;
    } && std::is_constructible_v<Storage, ErrTag, E> &&
    std::is_constructible_v<Storage, OkTag, T>;

template <class E>
struct RawError : RawStorage<E> {};

template <class T, class E>
struct ResultStorage : protected OptionStorage<T>, protected RawError<E> {
  public:
    bool is_ok() const noexcept { return this->is_some(); }
    using OptionStorage<T>::unwrap_unsafe;

    void swap(ResultStorage<T, E>& other) {
        const auto this_ok = this->is_ok();
        const auto other_ok = other.is_ok();

        OptionStorage<T>::swap(other);

        if (this_ok == other_ok) {
            if (!this_ok) {
                std::swap(this->unwrap_err_unsafe(), other.unwrap_err_unsafe());
            }
            return;
        }

        auto error_src = this;
        auto error_dst = &other;

        if (this_ok) {
            error_src = &other;
            error_dst = this;
        }

        new (error_dst->as_err_storage().get_bytes())
            E{std::move(*error_src->unwrap_err_unsafe())};
        error_src->unwrap_err_unsafe().~E();
    }

    E& unwrap_err_unsafe() & noexcept {
        return *this->as_err_storage().get_raw();
    }
    E&& unwrap_err_unsafe() && noexcept {
        return std::move(*this->as_err_storage().get_raw());
    }
    const E& unwrap_err_unsafe() const& noexcept {
        return *this->as_err_storage().get_raw();
    }

    template <class... Args>
    ResultStorage(OkTag, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        requires std::is_constructible_v<T, Args...>
        : OptionStorage<T>{Some, std::forward<Args>(args)...} {}

    template <class... Args>
    ResultStorage(ErrTag, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
        requires std::is_constructible_v<E, Args...>
        : OptionStorage<T>{None} {
        new (this->as_err_storage().get_bytes()) E{std::forward<Args>(args)...};
    }

    // -------- Copy constructors -------
    ResultStorage(const ResultStorage&) noexcept
        requires(std::is_trivially_copy_constructible_v<T>)
    = default;

    ResultStorage(const ResultStorage& other) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        requires(!std::is_trivially_copy_constructible_v<T>)
    {
        if (other.is_ok()) {
            new (this) ResultStorage{Ok, other.unwrap_unsafe()};
        } else {
            new (this) ResultStorage{Err, other.unwrap_err_unsafe()};
        }
    }

    // -------- Move constructors -------

    ResultStorage(ResultStorage&& other) noexcept
        requires(std::is_trivially_move_constructible_v<T>)
    = default;

    // moves and resets other storage!
    ResultStorage(ResultStorage&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>)
    {
        if (other.is_ok()) {
            new (this) ResultStorage{Ok, std::move(other).unwrap_unsafe()};
        } else {
            new (this) ResultStorage{Err, std::move(other).unwrap_err_unsafe()};
        }
    }

    // -------- Copy assignment -------

    ResultStorage& operator=(const ResultStorage&) noexcept
        requires(std::is_trivially_copy_assignable_v<T>)
    = default;

    ResultStorage& operator=(const ResultStorage& other)
        requires(!std::is_trivially_copy_assignable_v<T>)
    {
        ResultStorage tmp(other);
        this->swap(tmp);
        return *this;
    }

    // -------- Move assignment -------

    ResultStorage& operator=(ResultStorage&& other) noexcept
        requires(std::is_trivially_move_assignable_v<T>)
    = default;

    // moves and resets other storage!
    ResultStorage& operator=(ResultStorage&& other)
        requires(!std::is_trivially_move_assignable_v<T>)
    {
        ResultStorage tmp(std::move(other));
        this->swap(tmp);
        return *this;
    }

    // ------ Destructors ------
    ~ResultStorage()
        requires(std::is_trivially_destructible_v<E>)
    = default;

    ~ResultStorage() {
        if (!this->is_ok()) {
            this->unwrap_err_unsafe().~E();
        }
    }
    // -----------------------
  private:
    RawStorage<T>& as_storage() & { return *this; }
    RawStorage<E>& as_err_storage() & {
        return *static_cast<RawError<E>*>(this);
    }

    const RawStorage<T>& as_storage() const& { return *this; }
    const RawStorage<E>& as_err_storage() const& {
        return *static_cast<const RawError<E>*>(this);
    }

    ResultStorage() noexcept = default;
};

template <class T>
struct PrimitiveWrapper {
    T value;

    template <class... Args>
    PrimitiveWrapper(Args&&... args) : value{std::forward<Args>(args)...} {}

    PrimitiveWrapper() = default;

    operator T&() & { return value; }
    operator const T&() const& { return value; }
};

template <class T>
using MayBeWrapped =
    std::conditional_t<std::is_scalar_v<T> || std::is_final_v<T>,
                       PrimitiveWrapper<T>, T>;

template <class T>
struct ResultStorage<T, T> : private MayBeWrapped<T> {
    template <class... Args>
    ResultStorage(OkTag, Args&&... args)
        : MayBeWrapped<T>{std::forward<Args>(args)...}, _is_ok{true} {}

    template <class... Args>
    ResultStorage(ErrTag, Args&&... args)
        : MayBeWrapped<T>{std::forward<Args>(args)...}, _is_ok{false} {}

    void swap(ResultStorage& other) noexcept {
        std::swap(this->_is_ok, other._is_ok);
        std::swap(this->as_inner(), other.as_inner);
    }

    T& unwrap_unsafe() & noexcept { return as_inner(); }
    const T& unwrap_unsafe() const& noexcept { return as_inner(); }

    T& unwrap_err_unsafe() & noexcept { return as_inner(); }
    const T& unwrap_err_unsafe() const& noexcept { return as_inner(); }

    bool is_ok() const noexcept { return _is_ok; }

  private:
    T& as_inner() & { return *static_cast<MayBeWrapped<T>*>(this); }

    const T& as_inner() const& {
        return *static_cast<const MayBeWrapped<T>*>(this);
    }

    bool _is_ok;
};

} // namespace better