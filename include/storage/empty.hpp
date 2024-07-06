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

#include "generic.hpp"

namespace better {

// This specialization covers all empty trivial types, including
// better::Void

template <class T>
    requires std::is_trivial_v<T> && std::is_empty_v<T>
struct OptionStorage<T> : protected T {

    const T& unwrap_unsafe() const& { return *this; }
    T& unwrap_unsafe() & { return *this; }
    T&& unwrap_unsafe() && { return std::move(*this); }

    bool is_some() const noexcept { return _is_some; }
    void swap(OptionStorage& other) noexcept {
        std::swap(this->_is_some, other._is_some);
    }

    OptionStorage(NoneTag) noexcept : _is_some{false} {}
    template <class... Args>
    OptionStorage(SomeTag, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        : T{std::forward<Args>(args)...}, _is_some{true} {}

    OptionStorage(const OptionStorage&) = default;
    OptionStorage(OptionStorage&& other) noexcept
        : _is_some{std::exchange(other._is_some, false)} {}

    OptionStorage& operator=(const OptionStorage&) = default;
    OptionStorage& operator=(OptionStorage&& other) noexcept {
        OptionStorage tmp{std::move(other)};
        this->swap(tmp);
        return *this;
    }

    ~OptionStorage() = default;

  private:
    bool _is_some = false;
};

} // namespace better
