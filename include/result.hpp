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

#include "invoke_with.hpp"
#include "storage/generic_result.hpp"

#include <stdexcept>

namespace better {

template <class T>
struct Option;

template <class T, class E>
struct Result : protected ResultStorage<T, E> {
    static_assert(ResultStorageImpl<ResultStorage<T, E>, T, E>);
    static_assert(!std::is_const_v<E>, "const cvalified types are not allowed");
    static_assert(!std::is_same_v<E, void>,
                  "built-in void type cannot be supported as a type parameter. "
                  "Use better::Void");
    static_assert(!std::is_reference_v<E>,
                  "built-in reference types cannot be supported as a type "
                  "parameter. Use better::Ref");

    template <class... Args>
    Result(OkTag, Args&&... args)
        : ResultStorage<T, E>{Ok, std::forward<Args>(args)...} {}

    template <class... Args>
    Result(ErrTag, Args&&... args)
        : ResultStorage<T, E>{Err, std::forward<Args>(args)...} {}

    using ResultStorage<T, E>::is_ok;

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
            return std::move(this->unwrap_err_unsafe());
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    E& unwrap_err() & {
        if (is_err()) {
            return this->unwrap_err_unsafe();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    const E& unwrap_err() const& {
        if (is_err()) {
            return this->unwrap_err_unsafe();
        } else {
            throw std::runtime_error(
                "Attempt to unwrap_err Result that contains Ok");
        }
    }

    void swap(Result<T, E>& other) { ResultStorage<T, E>::swap(other); }

    template <class F>
        requires IsInvocableWith<F, T>
    auto map(F&& f) && {
        using R = decltype(invoke_with(std::forward<F>(f), std::declval<T>()));

        if (this->is_ok()) {
            return Result<R, E>{Ok,
                                invoke_with(std::forward<F>(f),
                                            std::move(this->unwrap_unsafe()))};
        } else {
            return Result<R, E>{Err, std::move(this->unwrap_err_unsafe())};
        }
    }

    template <class F>
        requires IsInvocableWith<F, const T&>
    auto map(F&& f) const& {
        using R =
            decltype(invoke_with(std::forward<F>(f), std::declval<const T&>()));

        if (this->is_ok()) {
            return Result<R, E>{
                Ok, invoke_with(std::forward<F>(f), this->unwrap_unsafe())};
        } else {
            return Result<R, E>{Err, *this->as_err_storage().get_raw()};
        }
    }

    template <class F>
        requires IsInvocableWith<F, E>
    auto map_err(F&& f) && {
        using NewE =
            decltype(invoke_with(std::forward<F>(f), std::declval<E>()));

        if (this->is_ok()) {
            return Result<T, NewE>{Ok, std::move(this->unwrap_unsafe())};
        } else {
            return Result<T, NewE>{
                Err, invoke_with(std::forward<F>(f),
                                 std::move(this->unwrap_err_unsafe()))};
        }
    }

    template <class F>
        requires IsInvocableWith<F, E>
    auto map_err(F&& f) const& {
        using NewE =
            decltype(invoke_with(std::forward<F>(f), std::declval<E>()));

        if (this->is_ok()) {
            return Result<T, NewE>{Ok, this->unwrap_unsafe()};
        } else {
            return Result<T, NewE>{Err, invoke_with(std::forward<F>(f),
                                                    this->unwrap_err_unsafe())};
        }
    }

    auto as_ref() & {
        using ResultRefT = Result<Ref<T>, Ref<E>>;
        if (this->is_ok()) {
            return ResultRefT{Ok, Ref<T>{this->unwrap_unsafe()}};
        } else {
            return ResultRefT{Err, Ref<E>{this->unwrap_err_unsafe()}};
        }
    }

    auto as_ref() const& {
        using NewT = MakeConstRefType<T>;
        using NewE = MakeConstRefType<E>;
        using ResultRefT = Result<NewT, NewE>;

        if (this->is_ok()) {
            return ResultRefT{Ok, NewT{this->unwrap_unsafe()}};
        } else {
            return ResultRefT{Err, NewE{this->unwrap_err_unsafe()}};
        }
    }

    Option<T> ok() && {
        if (this->is_ok()) {
            return Option<T>{Some, std::move(this->unwrap_unsafe())};
        } else {
            return Option<T>{None};
        }
    }

    Option<T> ok() const& {
        if (this->is_ok()) {
            return Option<T>{Some, this->unwrap_unsafe()};
        } else {
            return Option<T>{None};
        }
    }

    Option<E> err() && {
        if (this->is_err()) {
            return Option<E>{Some, std::move(this->unwrap_err_unsafe())};
        } else {
            return Option<E>{None};
        }
    }

    Option<E> err() const& {
        if (this->is_err()) {
            return Option<E>{Some, this->unwrap_err_unsafe()};
        } else {
            return Option<E>{None};
        }
    }

    template <class F>
    auto and_then(F&& f) &&
        requires IsInvocableWith<F, T> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward<F>(f),
                                          std::declval<T>())),
                     ErrTag, E>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward<F>(f), std::declval<T>()));
        return is_ok()
                   ? invoke_with(std::forward<F>(f),
                                 std::move(this->unwrap_unsafe()))
                   : ResultOpt{Err, std::move(this->unwrap_err_unsafe())};
    }

    template <class F>
    auto and_then(F&& f) const&
        requires IsInvocableWith<F, const T&> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward<F>(f),
                                          std::declval<const T&>())),
                     ErrTag, const E&>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward<F>(f), std::declval<T>()));
        return is_ok() ? invoke_with(std::forward<F>(f), this->unwrap_unsafe())
                       : ResultOpt{Err, this->unwrap_err_unsafe()};
    }

    template <class F>
    auto or_else(F&& f) &&
        requires IsInvocableWith<F, E> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward<F>(f), std::declval<E>())),
                     OkTag, T>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward<F>(f), std::declval<E>()));
        return is_err() ? invoke_with(std::forward<F>(f),
                                      std::move(this->unwrap_err_unsafe()))
                        : ResultOpt{Ok, std::move(this->unwrap_unsafe())};
    }

    template <class F>
    auto or_else(F&& f) const&
        requires IsInvocableWith<F, const E&> &&
                 std::is_constructible_v<
                     decltype(invoke_with(std::forward<F>(f),
                                          std::declval<const E&>())),
                     OkTag, T>
    {
        using ResultOpt =
            decltype(invoke_with(std::forward<F>(f), std::declval<const E&>()));
        return is_err()
                   ? invoke_with(std::forward<F>(f), this->unwrap_err_unsafe())
                   : ResultOpt{Ok, this->unwrap_unsafe()};
    }

  private:
    RawError<E>& as_err_storage() & { return *this; }

    const RawError<E>& as_err_storage() const& { return *this; }
};

} // namespace better