#include "option.hpp"

#include <iostream>
#include <string>
#include <vector>

using better::None;
using better::Option;
using better::Ref;
using better::Some;


void test_take_and_insert() {
    std::cout << "test take and insert";
    Option<std::vector<int>> opt_v = None;
    std::cout << opt_v.is_some();
    opt_v.insert(std::vector{1,2,3,4,5});
    std::cout << opt_v.is_some();
    auto opt_v2 = std::move(opt_v);
    std::cout << opt_v.is_some();
    std::cout << opt_v2.is_some();
    auto opt_v3 = opt_v2.take();
    opt_v3.as_ref().map([](auto v_ref) { v_ref->pop_back(); });
    auto v = std::move(opt_v3).unwrap();
    std::cout << "\n" << v.size() << "\n";
}


int main() {

    test_take_and_insert();

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


    std::string hello = "hello";

    Option<Ref<std::string>> opt_ref = { Some, Ref {world} };

    // we can take reference to reference!
    // and update reference
    opt_ref.as_ref().map([&](Ref<std::string>& r){ r = Ref{hello}; });

    // mutate hello 
    hello = "HI";

    opt_ref.map([](const std::string& s) {
        std::cout << "Mutated ref: " << s << "\n";
    });

    return 0;
}