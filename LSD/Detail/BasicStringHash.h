/*************************
 * @file BasicStringHash.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Base implementation for all string hashes
 * 
 * @date 2025-04-18
 * @copyright Copyright (c) 2025
 *************************/

#pragma once

#define LSD_STRHASH_64 UINTPTR_MAX == UINT64_MAX

#if !(LSD_STRHASH_64) && UINTPTR_MAX != UINT32_MAX
#error "lsd::basicStringHash(): The core hash function for strings only supports 64 or 32 bit systems!"
#endif

#include "../Array.h"

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <bit>

namespace lsd {

namespace detail {

namespace strhash {

static constexpr auto sizeOfSize = sizeof(std::size_t);


// Type aliases

using size32 = std::conditional_t<sizeOfSize == 8, std::uint32_t, std::size_t>;
using size64 = std::conditional_t<sizeOfSize == 8, std::size_t, std::uint64_t>;

using value_conditional = ValueConditional<sizeOfSize == 8, size64, size32>;


// Constants

inline constexpr auto fnvOffset = value_conditional::template get<size64 { 0xCBF29CE484222325 }, size32 { 0x811C9DC5 }>();
inline constexpr auto fnvPrime = value_conditional::template get<size64 { 0x100000001B3 }, size32 { 0x01000193 }>();

inline constexpr size64 hashOffset = 0x9E3779B185EBCA87;
inline constexpr size64 hashPrime = 0xC2B2AE3D27D4EB4F;




// Utility functions

template <std::size_t Count> inline constexpr size32 rotl(size32 n) {
	return (n << Count) | (n >> (Count - 1));
}

inline constexpr size64 mul128AndFold64(size64 a, size64 b) noexcept {
	auto aLo = a & 0xFFFFFFFF;
	a >>= 32;
	auto bLo = b & 0xFFFFFFFF;
	b >>= 32;

	auto p0 = aLo * bLo;
	auto p1 = a * b;
	auto cross = (aLo + a) * (bLo + b) - p1 - p0;

	auto low = p0 + (cross << 32);

	return low ^ (p1 + (cross >> 32) + (low < p0)); // Discards a overflow byte
}

template <std::integral Character> inline constexpr size32 read32(Character* it) noexcept {
	if constexpr (sizeof(Character) == 1) {
		auto a = static_cast<size32>(*it++);
		auto b = static_cast<size32>(*it++) << 8;
		auto c = static_cast<size32>(*it++) << 16;
		auto d = static_cast<size32>(*it) << 24;

		return a | b | c | d;
	} else if constexpr (sizeof(Character) == 2) {
		auto a = static_cast<size32>(*it++);
		auto b = static_cast<size32>(*it) << 16;
		
		return a | b;
	} else return static_cast<size32>(*it);
}

template <std::integral Character> inline constexpr size64 read64(Character* it) noexcept {
	if constexpr (sizeof(Character) < 8)
		return read32(it) | static_cast<size64>(read32(it + (4 / sizeof(Character)))) << 32;
	else return static_cast<size64>(*it++);
}

inline constexpr std::size_t avalanche(std::size_t hash) noexcept {
#if LSD_STRHASH_64
	hash = (hash ^ (hash >> 33)) * 0x165667919E3779F9;
	return hash ^ (hash >> 33);
#else
	hash = (hash ^ hash >> 16) * 0x85EBCA6B;
	hash = (hash ^ hash >> 13) * 0xC2B2AE35;
	return hash ^ hash >> 16;
#endif
}


inline constexpr size64 secretAt(std::size_t index) noexcept {
	lsd::Array<std::uint8_t, 128> secret { // Calculated from decimal digits of pi
		0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe,
		0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
		0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb,
		0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
		0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78,
		0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
		0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e,
		0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
		0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb,
		0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
		0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e,
		0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
		0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f,
		0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
		0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31,
		0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64
	};

	return read64(secret.data() + (index & secret.size()));
}

template <std::integral Character> inline constexpr std::size_t mix16Bytes(Character* it, std::size_t secretIndex) noexcept {
	auto a = read64(it) ^ secretAt(secretIndex);
	auto b = read64(it + (8 / sizeof(Character))) ^ secretAt(secretIndex + 8);

#if LSD_STRHASH_64
	return mul128AndFold64(a, b);
#else
	return (rotl<1>(a & 0xFFFFFFFF) + rotl<7>(a >> 32) + rotl<12>(b & 0xFFFFFFFF) + rotl<18>(a >> 32)) - secretIndex;
#endif
}

}

template <std::integral Character> inline constexpr std::size_t basicStringHash(Character* str, std::size_t count) noexcept {
	using namespace strhash;

	static constexpr auto blockCount = 16 / sizeof(Character);

	if (count <= 16) { // Use FNV-1a for smaller strings, not related to blockCount
		auto hash = fnvOffset;

		auto last = *str;
		while (count--) {
			auto current = *str++;
			hash = ((hash ^ current) * fnvPrime) ^ last; // Slightly modifies the algorithm to introduce a backward dependency
			last = current;
		}

		return avalanche(hash);
	} else { // I have no idea if this is even good
		std::size_t hash = count * hashOffset;

		auto countCopy = count;
		for (std::size_t i = 0, s = 0; count >= blockCount; count -= blockCount, i += blockCount, ++s) {
			hash += hashPrime;
			hash ^= mix16Bytes(str + i, s);
		}

		hash += hashPrime;
		hash ^= mix16Bytes(str + countCopy - blockCount, count);

		return avalanche(hash);
	}
}

} // namespace detail

} // namespace lsd
