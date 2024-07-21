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

#include "raw.hpp"

#include "../ref.hpp"
#include "../tags.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace better {

// Mandatory interface for OptionStorage:
// swap(Other)
// is_some()
// unwrap_unsafe()
//
// and two constructors:
// OptionStorage(NoneTag)
// OptionStorage(SomeTag, ...)
// Specializations must implement them all
template <class Storage, class T>
concept OptionStorageImpl =
    requires(Storage& mut_storage, const Storage& const_storage) {
        { const_storage.is_some() } -> std::same_as<bool>;
        { mut_storage.swap(mut_storage) };

        { mut_storage.unwrap_unsafe() } -> IsLvalueReference;
        { const_storage.unwrap_unsafe() } -> IsLvalueReference;
    } && std::is_constructible_v<Storage, NoneTag> &&
    std::is_constructible_v<Storage, SomeTag, T>;

template <class T>
struct OptionStorage : private RawStorage<T> {
  public:
    bool is_some() const noexcept { return _initialized; }

    void swap(OptionStorage<T>& other) noexcept(
        std::is_trivially_move_constructible_v<T> ||
        std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_move_constructible_v<T>) {
            std::swap(this->as_storage(), other.as_storage());
            std::swap(this->_initialized, other._initialized);
            return;
        } else {
            if (other._initialized && this->_initialized) {
                std::swap(this->unwrap_unsafe(), other.unwrap_unsafe());
                return;
            }
            if (other._initialized) {
                new (this)
                    OptionStorage{Some, std::move(other).unwrap_unsafe()};
                other.reset();
                return;
            }
            if (this->_initialized) {
                new (&other)
                    OptionStorage{Some, std::move(this->unwrap_unsafe())};
                this->reset();
                return;
            }
        }
        // both None, do nothing
    }

    T& unwrap_unsafe() & noexcept { return *this->get_raw(); }
    T&& unwrap_unsafe() && noexcept { return std::move(*this->get_raw()); }
    const T& unwrap_unsafe() const& noexcept { return *this->get_raw(); }

    OptionStorage(NoneTag) noexcept : OptionStorage() {}

    template <class... Args>
    OptionStorage(SomeTag, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        requires std::is_constructible_v<T, Args...>
        : RawStorage<T>{InitializeTag{}, std::forward<Args>(args)...},
          _initialized{true} {}

    // -------- Copy constructors -------
    OptionStorage(const OptionStorage&) noexcept
        requires(std::is_trivially_copy_constructible_v<T>)
    = default;

    OptionStorage(const OptionStorage& other) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        requires(!std::is_trivially_copy_constructible_v<T>)
    {
        if (other.is_some()) {
            new (this) OptionStorage{Some, other.unwrap_unsafe()};
        }
    }

    // -------- Move constructors -------

    OptionStorage(OptionStorage&& other) noexcept
        requires(std::is_trivially_move_constructible_v<T>)
    = default;

    // moves and resets other storage!
    OptionStorage(OptionStorage&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>)
    {
        if (other.is_some()) {
            new (this) OptionStorage{Some, std::move(other).unwrap_unsafe()};
        }
    }

    // -------- Copy assignment -------

    OptionStorage& operator=(const OptionStorage&) noexcept
        requires(std::is_trivially_copy_assignable_v<T>)
    = default;

    OptionStorage& operator=(const OptionStorage& other) noexcept(
        std::is_nothrow_copy_constructible_v<T> &&
        noexcept(this->swap(std::declval<OptionStorage&>())))
        requires(!std::is_trivially_copy_assignable_v<T>)
    {

        OptionStorage tmp(other);
        this->swap(tmp);

        return *this;
    }

    // -------- Move assignment -------

    OptionStorage& operator=(OptionStorage&& other) noexcept
        requires(std::is_trivially_move_assignable_v<T>)
    = default;

    // moves and resets other storage!
    OptionStorage& operator=(OptionStorage&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        noexcept(this->swap(std::declval<OptionStorage&>())))
        requires(!std::is_trivially_move_assignable_v<T>)
    {
        OptionStorage tmp(std::move(other));
        this->swap(tmp);

        return *this;
    }

    // ------ Destructors ------
    ~OptionStorage()
        requires(std::is_trivially_destructible_v<T>)
    = default;

    ~OptionStorage() noexcept(std::is_nothrow_destructible_v<T>)
        requires(!std::is_trivially_destructible_v<T>)
    {
        reset();
    }
    // -----------------------
  private:
    RawStorage<T>& as_storage() & { return *this; }

    OptionStorage() noexcept = default;

    void reset() noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            if (_initialized) {
                this->get_raw()->~T();
            }
        }
        _initialized = false;
    }

    bool _initialized = false;
};

} // namespace better