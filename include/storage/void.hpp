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

#include "../void.hpp"

namespace better {

// Void specialization -- this one is easy
// Derive from Void to be able to form a reference to it without unsafe
// Void is Zero sized, so Empty Base Optimization will take place
template <>
struct OptionStorage<Void> : protected Void {

    const Void& unwrap_unsafe() const& { return *this; }
    Void& unwrap_unsafe() & { return *this; }
    Void&& unwrap_unsafe() && { return std::move(*this); }

    bool is_some() const noexcept { return _is_some; }
    void swap(OptionStorage& other) noexcept {
        std::swap(this->_is_some, other._is_some);
    }

    OptionStorage(NoneTag) noexcept : _is_some{false} {}
    OptionStorage(SomeTag, auto&&...) noexcept : _is_some{true} {}

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