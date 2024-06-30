#include "option.hpp"

#include <iostream>
#include <string>

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