#include <option.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

using better::None;
using better::Option;
using better::Ref;
using better::Some;

std::string random_string(size_t len) {
    static std::random_device rd;  // a seed source for the random number engine
    static std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    static std::uniform_int_distribution<uint32_t> distrib('a', 'z');

    std::string s;
    s.resize(len);
    std::generate_n(s.data(), s.size(), []() -> char { return distrib(gen); });
    return s;
}

template <class F> auto time(std::string_view title, F &&f) -> uint64_t {
    // std::cout << "testing: " << title << "\n";
    const auto start = std::chrono::high_resolution_clock::now();
    auto ret = std::forward<F>(f)();
    const auto finish = std::chrono::high_resolution_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(finish - start)
            .count();
    // std::cout << "return value: " << ret << "\n";
    return elapsed;
}

size_t test_std_optional_refs(const std::vector<std::string> &str) {
    std::vector<std::optional<std::reference_wrapper<const std::string>>> opts;
    opts.reserve(str.size());

    for (const auto &s : str) {
        opts.emplace_back(std::in_place, s);
    }

    size_t sum_of_lens = 0;
    for (const auto &s : opts) {
        sum_of_lens += s->get().length();
    }
    return sum_of_lens;
}

size_t test_better_optional_refs(const std::vector<std::string> &str) {
    std::vector<Option<Ref<const std::string>>> opts;
    opts.reserve(str.size());

    for (const auto &s : str) {
        opts.emplace_back(Some, Ref{s});
    }

    size_t sum_of_lens = 0;
    for (auto s : opts) {
        std::move(s).map(
            [&](const std::string &s) { sum_of_lens += s.length(); });
    }
    return sum_of_lens;
}

void bench_references() {
    const size_t N = 10000;
    std::vector<std::string> strs;
    strs.reserve(N);
    std::generate_n(std::back_inserter(strs), N,
                    [] { return random_string(10); });

    const size_t RUNS = 1000;
    std::vector<uint64_t> measurements(RUNS);
    //
    for (auto &m : measurements) {
        m = time("better::Option",
                 [&] { return test_better_optional_refs(strs); });
        // m = time("std::optional", [&] { return test_std_optional_refs(strs);
        // });
    }
    std::cout << "better::Option\n";
    // std::cout << "std::optional\n";
    std::sort(measurements.begin(), measurements.end());
    std::cout << "Elapsed min: " << measurements[0] << " usec\n";
    std::cout << "Elapsed p50: " << measurements[RUNS / 2] << " usec\n";
    std::cout << "Elapsed p90: " << measurements[90 * RUNS / 100] << " usec\n";
    std::cout << "Elapsed p99: " << measurements[99 * RUNS / 100] << " usec\n";
    std::cout << "Elapsed max: " << measurements.back() << " usec\n";
}

int main() { bench_references(); };