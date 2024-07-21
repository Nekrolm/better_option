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

#include <type_traits>

namespace better {
    

template <class T>
struct RawStorage {
    alignas(T) std::byte data[sizeof(T)];

    char* get_bytes() noexcept { return reinterpret_cast<char*>(data); }

    T* get_raw() noexcept {
        return std::launder(reinterpret_cast<T*>(data));
    }
    const T* get_raw() const noexcept {
        return std::launder(reinterpret_cast<const T*>(data));
    };
};

template<class T>
requires std::is_trivial_v<T> && std::is_empty_v<T>
struct RawStorage<T>: private T {
    char* get_bytes() noexcept { return reinterpret_cast<char*>(this); }

    T* get_raw() noexcept {
        return this;
    }
    const T* get_raw() const noexcept {
        return this;
    };
};

}