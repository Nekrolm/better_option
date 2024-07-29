#include "option.hpp"
#include "result.hpp"
#include "void.hpp"

#include <iostream>
#include <string>
#include <vector>

using better::Err;
using better::None;
using better::Ok;
using better::Ref;
using better::Result;
using better::Some;
using better::Void;

void test_result_and_then() {
    std::cout << "test_result_and_then\n";
    Result<int, std::string> res = {Ok, 55};
    Result<int, std::string> err = {Err, "world"};

    auto ok_val =
        res.and_then([](int x) { return Result<Void, std::string_view>{Ok}; });
    auto err_val =
        err.and_then([](int x) { return Result<Void, std::string_view>{Ok}; });

    std::cout << "ok_value is ok: " << ok_val.is_ok() << "\n";
    std::cout << "err_val is err: " << err_val.is_err() << "\n";

    auto err_msg = err_val.unwrap_err();
    std::cout << "err_val message: " << err_msg << "\n";
}

void test_result_or_else() {
    std::cout << "test_result_or_else\n";
    Result<int, std::string> res = {Ok, 55};
    Result<int, std::string> err = {Err, "world"};

    auto ok_val = res.or_else([](std::string_view x) {
        return Result<ssize_t, std::string_view>{
            Ok, static_cast<ssize_t>(x.length())};
    });
    auto err_val = err.or_else([](std::string_view x) {
        return Result<int, std::string_view>{Err, x};
    });

    std::cout << "ok_value is ok: " << ok_val.is_ok() << "\n";
    std::cout << "err_val is err: " << err_val.is_err() << "\n";

    auto err_msg = err_val.unwrap_err();
    std::cout << "err_val message: " << err_msg << "\n";
    auto ok_x = ok_val.unwrap();
    std::cout << "ok_val: " << ok_x << "\n";
}

int main() {

    test_result_and_then();
    test_result_or_else();

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

    Result<std::string, std::string> err_str = {Err, "error"};

    auto ref_err = std::as_const(err_str).as_ref();

    auto mapped_err = ref_err.map_err(
        [](const std::string& err_s) { return err_s.length(); });

    static_assert(std::is_same_v<decltype(mapped_err),
                                 Result<Ref<const std::string>, size_t>>);

    std::cout << "mapped_err: " << mapped_err.unwrap_err() << "\n";

    struct EmptyErr {};
    static_assert(sizeof(Result<int, EmptyErr>) == 2 * sizeof(int));
    static_assert(sizeof(Result<Void, EmptyErr>) == sizeof(bool));

    static_assert(sizeof(Result<int, int>) == 2 * sizeof(int));

    mapped_err.ok();

    return 0;
}
