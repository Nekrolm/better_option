# Better Option 

This is a small header only library with implementation of  C++17 `std::optional`

## Main differences

1. It supports references!. Via custom Ref wrapper. Similar to std::reference_wrapper but simplier
2. It has niche optimization for references: `sizeof(Option<Ref<T>>) == sizeof(T*)`
3. It supports `void` via custom `Void` type. We can `.map` with functions returning `void` and also `Option<Void>` can be mapped with function accepting no arguments
4. C++20.

```C++
using better::None;
using better::Option;
using better::Ref;
using better::Some;

int main() {

    Option<std::string> opt = {Some, "hello world"};

    auto transformed = std::move(opt)
                           .map([](auto &&str) { return str.length(); })
                           .map([](size_t len) { return std::to_string(len); });

    // can map to void!
    auto result = transformed.as_ref().map(
        [](const auto &str) { std::cout << *str << "\n"; });

    // we have niche optimization for references
    static_assert(sizeof(transformed.as_ref()) == sizeof(void*));

    std::cout << "transformed is stil some: " << transformed.is_some() << "\n";

    std::string world = "world";

    std::cout << result.is_some() << "\n";
    static_assert(sizeof(result) == sizeof(bool), "Option<Void> is one byte only");

    // can also map from void!
    std::move(result)
        .map([&]() -> std::string & { return world; })
        .map([](auto ref) { std::cout << "got ref: " << *ref << "\n"; });

    return 0;
}
```