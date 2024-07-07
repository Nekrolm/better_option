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

#include "option.hpp"

namespace better {

template <class T, class E>
struct Result : protected Option<T> {
    static_assert(!std::is_const_v<E>, "const cvalified types are not allowed");
    static_assert(!std::is_same_v<E, void>,
                  "built-in void type cannot be supported as a type parameter. "
                  "Use better::Void");
    static_assert(!std::is_reference_v<E>,
                  "built-in reference types cannot be supported as a type "
                  "parameter. Use better::Ref");

    template <class... Args>
    Result(OkTag, Args&&... args)
        : Option<T>{Some, std::forward<Args>(args)...} {}

    template <class... Args>
    Result(ErrTag, Args&&... args) : Option<T>{None} {
        new (_error.get_bytes()) E{std::forward<Args>(args)...};
    }

    ~Result()
        requires std::is_trivially_destructible_v<E>
    = default;

    ~Result() noexcept(std::is_nothrow_destructible_v<E>) {
        if (is_err()) {
            _error.get_raw()->~E();
        }
    }

    // ---- Copy Constructors ----

    Result(const Result&)
        requires std::is_trivially_copy_constructible_v<E>
    = default;

    Result(const Result& other) noexcept(
        std::is_nothrow_copy_constructible_v<Option<T>> &&
        std::is_nothrow_copy_constructible_v<E>)
        : Option<T>(other) {
        if (other.is_err()) {
            new (this->_error.get_bytes()) E{*other._error.get_raw()};
        }
    }

    // ---- Move Constructors ----

    Result(Result&&)
        requires std::is_trivially_move_constructible_v<E>
    = default;

    Result(Result&& other) noexcept(
        std::is_nothrow_move_constructible_v<Option<T>> &&
        std::is_nothrow_move_constructible_v<E>)
        : Option<T>(std::move(other)) {
        if (other.is_err()) {
            new (this->_error.get_bytes())
                E{std::move(*other._error.get_raw())};
        }
    }

    // ---- Copy assignment -----
    Result& operator=(const Result&)
        requires std::is_trivially_copy_assignable_v<E>
    = default;

    Result& operator=(const Result& other) {
        Result tmp{other};
        this->swap(tmp);
        return *this;
    }

    // ---- Move assignment -----
    Result& operator=(Result&&)
        requires std::is_trivially_move_assignable_v<E>
    = default;

    Result& operator=(Result&& other) {
        Result tmp{std::move(other)};
        this->swap(tmp);
        return *this;
    }

    bool is_ok() const { return this->is_some(); }
    bool is_err() const { return !this->is_ok(); }

    T&& unwrap() && {
        if (is_ok()) {
            return std::move(this->unwrap_unsafe());
        } else {
            throw std::runtime_error(
                "Attempt to unwrap Result that contains Err");
        }
    }

    T& unwrap() & {
        if (is_ok()) {
            return this->unwrap_unsafe();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap Result that contains Err");
        }
    }

    const T& unwrap() const& {
        if (is_ok()) {
            return this->unwrap_unsafe();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap Result that contains Err");
        }
    }

    E&& unwrap_err() && {
        if (is_err()) {
            return std::move(*_error.get_raw());
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    E& unwrap_err() & {
        if (is_err()) {
            return *_error.get_raw();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    const E& unwrap_err() const& {
        if (is_err()) {
            return *_error.get_raw();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    void swap(Result<T, E>& other) {
        const auto this_ok = this->is_ok();
        const auto other_ok = other.is_ok();

        Option<T>::swap(other);

        if (this_ok == other_ok) {
            if (!this_ok) {
                std::swap(*this->_error.get_raw(), *other._error.get_raw());
            }
            return;
        }

        auto error_src = this;
        auto error_dst = &other;

        if (this_ok) {
            error_src = &other;
            error_dst = this;
        }

        new (error_dst->_error.get_bytes())
            E{std::move(*error_src->_error.get_raw())};
        error_src->_error.get_raw()->~E();
    }

    template <class F>
        requires IsInvocableWith<F, T>
    auto map(F&& f) && {
        using R = decltype(invoke_with(std::forward<F>(f), std::declval<T>()));

        auto new_option =
            static_cast<Option<T>&&>(*this).map(std::forward<F>(f));

        if (new_option.is_some()) {
            return Result<R, E>{Some, std::move(new_option)};
        } else {
            return Result<R, E>{Err, std::move(*this->_error.get_raw())};
        }
    }

    template <class F>
        requires IsInvocableWith<F, const T&>
    auto map(F&& f) const& {
        using R =
            decltype(invoke_with(std::forward<F>(f), std::declval<const T&>()));

        auto new_option = Option<T>::map(std::forward<F>(f));

        if (new_option.is_some()) {
            return Result<R, E>{Some, std::move(new_option)};
        } else {
            return Result<R, E>{Err, *this->_error.get_raw()};
        }
    }

    auto as_ref() & {
        using ResultRefT = Result<Ref<T>, Ref<E>>;
        auto new_option = Option<T>::as_ref();
        if (new_option.is_some()) {
            return ResultRefT{Some, std::move(new_option)};
        } else {
            return ResultRefT{Err, Ref<E>{*this->_error.get_raw()}};
        }
    }

    auto as_ref() const& {
        using NewT = MakeConstRefType<T>;
        using NewE = MakeConstRefType<E>;
        using ResultRefT = Result<NewT, NewE>;

        auto new_option = Option<T>::as_ref();

        if (new_option.is_some()) {
            return ResultRefT{Some, std::move(new_option)};
        } else {
            return ResultRefT{Err, NewE{*this->_error.get_raw()}};
        }
    }

  private:
    template <class T1, class E1>
    friend struct Result;

    Result(SomeTag, Option<T>&& base) : Option<T>{std::move(base)} {}

    struct RawStorage {
        alignas(E) std::byte data[sizeof(E)];

        char* get_bytes() noexcept { return reinterpret_cast<char*>(data); }

        E* get_raw() noexcept {
            return std::launder(reinterpret_cast<E*>(data));
        }
        const E* get_raw() const noexcept {
            return std::launder(reinterpret_cast<const E*>(data));
        }
    };
    RawStorage _error;
};

} // namespace better