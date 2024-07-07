#include "result.hpp"

#include <iostream>
#include <string>
#include <vector>

using better::Err;
using better::None;
using better::Ok;
using better::Option;
using better::Ref;
using better::Result;
using better::Some;

int main() {

    Result<int, std::string> res = {Ok, 55};
    Result<int, std::string> err = {Err, "hello"};

    std::cout << res.is_ok() << "\n";
    std::cout << err.is_ok() << "\n";

    auto mapped = res.map([](int x) { return std::to_string(x); });

    mapped.as_ref().map([](const std::string& s) {
        std::cout << "mapped string " << s << "\n";
    });

    Result<std::string, std::string> r_str = {Ok, "hello"};

    auto ref_r_str = r_str.as_ref();
    static_assert(std::is_same_v<decltype(ref_r_str),
                                 Result<Ref<std::string>, Ref<std::string>>>);

    auto const_ref_ref_r_str = std::as_const(ref_r_str).as_ref();
    static_assert(std::is_same_v<decltype(const_ref_ref_r_str),
                                 Result<Ref<const Ref<const std::string>>,
                                        Ref<const Ref<const std::string>>>>);

    auto ref = const_ref_ref_r_str.unwrap();

    return 0;
}
