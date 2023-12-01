#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <fstream>
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/preprocessor.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <Eigen/Dense>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#ifndef __CUDACC__
#include <boost/multiprecision/cpp_int.hpp>
using u128 = boost::multiprecision::uint128_t;
using s128 = boost::multiprecision::int128_t;

#include <fmt/ostream.h>

#include <experimental/mdspan>

#include <range/v3/all.hpp>
namespace views = ranges::views;

#include <immintrin.h>
#endif

using namespace std::literals;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using ssize = std::make_signed_t<usize>;

constexpr s64 operator""_s64(unsigned long long x) {
    return static_cast<s64>(x);
}

constexpr u64 operator""_u64(unsigned long long x) {
    return static_cast<u64>(x);
}

constexpr usize operator""_sz(unsigned long long x) {
    return static_cast<usize>(x);
}

template <typename I>
constexpr I AlignUp(I x, I alignment) {
    x = x + alignment - 1;
    return x - (x % alignment);
}

template <typename I>
constexpr I DivCeil(I x, I y) {
    return (x + y - 1) / y;
}

template <typename I, typename P>
constexpr I Power(I x, P y) {
    I result = 1;
    while (y != 0) {
        if (y & 1) result *= x;
        x *= x;
        y >>= 1;
    }
    return result;
}

template <typename T>
T sign(T val) {
    return (T(0) < val) - (val < T(0));
}

#ifndef __CUDACC__
inline std::bitset<8> rotr(std::bitset<8> x, int s) {
    return std::rotr(static_cast<u8>(x.to_ulong()), s);
}
inline std::bitset<8> rotl(std::bitset<8> x, int s) {
    return std::rotl(static_cast<u8>(x.to_ulong()), s);
}
inline std::bitset<16> rotr(std::bitset<16> x, int s) {
    return std::rotr(static_cast<u16>(x.to_ulong()), s);
}
inline std::bitset<16> rotl(std::bitset<16> x, int s) {
    return std::rotl(static_cast<u16>(x.to_ulong()), s);
}
inline std::bitset<32> rotr(std::bitset<32> x, int s) {
    return std::rotr(static_cast<u32>(x.to_ulong()), s);
}
inline std::bitset<32> rotl(std::bitset<32> x, int s) {
    return std::rotl(static_cast<u32>(x.to_ulong()), s);
}
inline std::bitset<64> rotr(std::bitset<64> x, int s) {
    return std::rotr(static_cast<u64>(x.to_ullong()), s);
}
inline std::bitset<64> rotl(std::bitset<64> x, int s) {
    return std::rotl(static_cast<u64>(x.to_ullong()), s);
}
#endif

#define VARIADIC_MAP(r, macro, i, elem) BOOST_PP_COMMA_IF(i) macro(elem)
#define __SCANNER_PASTE_VAL(member) val.member
#define SCANNER(T, fmt, ...)                                                                                           \
    template <>                                                                                                        \
    struct scn::scanner<T> : scn::empty_parser {                                                                       \
        template <typename Context>                                                                                    \
        error scan(T& val, Context& ctx) {                                                                             \
            return scn::scan_usertype(                                                                                 \
                ctx, (fmt),                                                                                            \
                BOOST_PP_SEQ_FOR_EACH_I(VARIADIC_MAP, __SCANNER_PASTE_VAL, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));    \
        }                                                                                                              \
    };

#ifdef _MSC_VER
#define UB() __assume(false)
#else
#define UB() __builtin_unreachable()
#endif

constexpr bool IsGraph(char c) {
    return c >= 33 && c <= 126;
}

constexpr bool IsDigit(char c) {
    return c >= (int)'0' && c <= (int)'9';
}

constexpr auto ChunkToStringView = views::transform([](ranges::random_access_range auto&& chunk) {
    return std::string_view{&*ranges::begin(chunk), static_cast<std::size_t>(ranges::distance(chunk))};
});

constexpr auto Split =
    views::split_when([](ranges::bidirectional_iterator auto begin, ranges::bidirectional_iterator auto end) {
        auto whitespace_end = std::find_if(begin, end, IsGraph);
        return std::make_pair(begin != whitespace_end, whitespace_end);
    }) |
    ChunkToStringView;

inline void ThrowErrc(std::errc errc) {
    throw std::runtime_error{std::make_error_condition(errc).message()};
}

template <typename T>
constexpr auto ParseNumbers =
    Split | views::transform([](std::string_view str) {
        T val{};
        if (str.front() == '+') str.remove_prefix(1);
        if (const std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), val);
            result.ec != std::errc{}) [[unlikely]] {
            ThrowErrc(result.ec);
        }
        return val;
    });

///////////////////////////////////////////////////////////////////////////////////////////////////

void AocMain(std::string input);

int main(const int argc, const char* argv[]) {
    std::ifstream input_file{"input.txt", std::ios::binary};
    std::string input{std::istreambuf_iterator{input_file}, {}};
    auto start = std::chrono::steady_clock::now();
    AocMain(std::move(input));
    auto end = std::chrono::steady_clock::now();
    fmt::print("Solved in {}\n", std::chrono::duration<double, std::milli>{end - start});
}