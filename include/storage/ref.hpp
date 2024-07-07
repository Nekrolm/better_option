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

template <class T>
struct OptionStorage<Ref<T>> {

    bool is_some() const noexcept { return storage.raw != nullptr; }

    Ref<T>& unwrap_unsafe() & noexcept { return storage.ref; }

    const typename Ref<T>::Const& unwrap_unsafe() const& noexcept {
        return storage.ref;
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

  private:
    union RawStorage {
        Ref<T> ref;
        T* raw;
    } storage;

    explicit OptionStorage(RawStorage raw) noexcept : storage{raw} {
        static_assert(sizeof(RawStorage) == sizeof(T*));
    }
};
} // namespace better
