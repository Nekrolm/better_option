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

#include "../ref.hpp"
#include "../tags.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace better {

template <class T>
concept IsLvalueReference =
    std::is_reference_v<T> && std::is_lvalue_reference_v<T>;

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
struct OptionStorage {
  public:
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

    struct RawStorage {
        alignas(T) std::byte data[sizeof(T)];

        char* get_bytes() noexcept { return reinterpret_cast<char*>(data); }

        T* get_raw() noexcept {
            return std::launder(reinterpret_cast<T*>(data));
        }
        const T* get_raw() const noexcept {
            return std::launder(reinterpret_cast<const T*>(data));
        }
    };

    RawStorage _storage;
    bool _initialized = false;
};

} // namespace better