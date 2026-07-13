/*************************
 * @file BasicStringHash.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Base implementation for all string hashes
 * 
 * @date 2025-04-18
 * @copyright Copyright (c) 2025
 *************************/

#pragma once

#define LSD_DETAIL_STRHASH_64 UINTPTR_MAX == UINT64_MAX

#if !(LSD_DETAIL_STRHASH_64) && UINTPTR_MAX != UINT32_MAX
#error "lsd::basicStringHash(): The core hash function for strings only supports 64 or 32 bit systems!"
#endif


#include "../CoreUtility.hpp"
#include "../../Array.h"

#include <cstddef>
#include <cstdint>
#include <iterator>


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

template <std::input_iterator Iterator> inline constexpr size32 read32(Iterator& it) noexcept {
	using char_type = std::iterator_traits<Iterator>::value_type;

	// Optimized path which allows for memcpy if the function is not compile time and the iterator is contiguous
	if constexpr (std::contiguous_iterator<Iterator> and not ReverseIterator<Iterator> and sizeof(char_type) < sizeof(size32)) {
		if !consteval {
			size32 result { };
			std::memcpy(&result, &*it, sizeof(size32));
			it += sizeof(size32) / sizeof(char_type);
			return result;
		}
	}

	// If the function is constexpr or if the iterator is not contiguous, this more explicit implementation is executed
	if constexpr (sizeof(char_type) == 1) {
		const auto c1 = static_cast<size32>(*it++);
		const auto c2 = static_cast<size32>(*it++) << 8;
		const auto c3 = static_cast<size32>(*it++) << 16;
		const auto c4 = static_cast<size32>(*it++) << 24;
		return c1 | c2 | c3 | c4;
	} else if constexpr (sizeof(char_type) == 2) {
		const auto c1 = static_cast<size32>(*it++);
		const auto c2 = static_cast<size32>(*it++) << 16;
		return c1 | c2;
	} else return static_cast<size32>(*it++);
}

template <std::input_iterator Iterator> inline constexpr size64 read64(Iterator& it) noexcept {
	using char_type = std::iterator_traits<Iterator>::value_type;

	if constexpr (std::contiguous_iterator<Iterator> and not ReverseIterator<Iterator> and sizeof(char_type) < sizeof(size64)) {
		if !consteval {
			size64 result { };
			std::memcpy(&result, &*it, sizeof(size64));
			it += sizeof(size64) / sizeof(char_type);
			return result;
		}
	}

	if constexpr (sizeof(char_type) == 1) {
		const auto c1 = static_cast<size64>(*it++);
		const auto c2 = static_cast<size64>(*it++) << 8;
		const auto c3 = static_cast<size64>(*it++) << 16;
		const auto c4 = static_cast<size64>(*it++) << 24;
		const auto c5 = static_cast<size64>(*it++) << 32;
		const auto c6 = static_cast<size64>(*it++) << 40;
		const auto c7 = static_cast<size64>(*it++) << 48;
		const auto c8 = static_cast<size64>(*it++) << 56;
		return c1 | c2 | c3 | c4 | c5 | c6 | c7 | c8;
	} if constexpr (sizeof(char_type) == 2) {
		const auto c1 = static_cast<size64>(*it++);
		const auto c2 = static_cast<size64>(*it++) << 16;
		const auto c3 = static_cast<size64>(*it++) << 32;
		const auto c4 = static_cast<size64>(*it++) << 48;
		return c1 | c2 | c3 | c4;
	} else if constexpr (sizeof(char_type) == 4) {
		const auto c1 = static_cast<size64>(*it++);
		const auto c2 = static_cast<size64>(*it++) << 32;
		return c1 | c2;
	} else return static_cast<size64>(*it++);
}


inline constexpr size64 secretAt64(std::size_t index) noexcept {
	static constexpr lsd::Array<std::uint8_t, 131> secret { // Table also from the xxHash implementation
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
		0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
		0x06, 0x0e, 0x29, // Last numbers added to complete the modulo hack
	};

	auto iter = secret.begin() + (index & size64 { 0x7F }); // Integer modulo hack
	return read64(iter);
}


template <std::size_t Count> inline constexpr std::size_t rotl(std::size_t n) {
	return (n << Count) | (n >> (Count - 1));
}

inline constexpr size64 mul128AndFold64(size64 a, size64 b) noexcept {
	auto aLo = a & 0xFFFFFFFF; a >>= 32;
	auto bLo = b & 0xFFFFFFFF; b >>= 32;

	auto z1 = (aLo + a) * (bLo + b);

	auto z0 = aLo * bLo;
	auto z2 = a * b;

	z1 = z1 - z0 - z2;

	return (z0 ^ (z1 << 32)) ^ ((z1 >> 32) ^ z2);
}


template <std::input_iterator Iterator> inline constexpr std::size_t mix16Bytes(Iterator it, std::size_t secretIndex) noexcept {
	using char_type = std::iterator_traits<Iterator>::value_type;

#if LSD_DETAIL_STRHASH_64
	// Maybe beneficial for OOO

	auto a = read64(it);
	auto sa = secretAt64(secretIndex);
	auto sb = secretAt64(secretIndex + 8);
	auto b = read64(it);

	return mul128AndFold64(a ^ sa, b ^ sb);
#else
	auto a = rotl<1>(read32(it));
	auto b = rotl<7>(read32(it));
	auto c = rotl<12>(read32(it));

	return a + b + c + rotl<16>(read32(it)) - secretIndex;
#endif
}


// Mix upper bits with lower bits of hash to improve avalanche effect
inline constexpr std::size_t avalanche(std::size_t hash) noexcept {
#if LSD_DETAIL_STRHASH_64
	hash = (hash ^ (hash >> 33)) * 0x165667919E3779F9;
	return hash ^ (hash >> 33);
#else
	hash = (hash ^ (hash >> 16)) * 0x85EBCA6B;
	hash = (hash ^ (hash >> 13)) * 0xC2B2AE35;
	return hash ^ (hash >> 16);
#endif
}

} // namespace strhash


template <std::input_iterator Iterator> inline constexpr std::size_t basicStringHash(Iterator begin, Iterator end) noexcept
	requires(std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>)
{
	using char_type = std::iterator_traits<Iterator>::value_type;

	using namespace strhash;

	static constexpr auto blockCount = 16 / sizeof(char_type);

	auto count = std::distance(begin, end);
	std::size_t hash = 0;
	if (count >= blockCount) { // xxHash variant
		hash = count * hashOffset;

		std::size_t i = 0;
		do {
			hash ^= mix16Bytes(begin, i++);
			hash = rotl<27>(hash) * hashPrime;
			
			std::advance(begin, blockCount);
		} while ((count -= blockCount) >= blockCount);
	}

	if (count > 0) { // Use FNV-1a for strings smaller than the 16-byte mixer 
		hash ^= fnvOffset;

		hash = (hash ^ *begin) * fnvPrime;

		for (auto last = ++begin; begin != end; begin++, last++)
			hash = ((hash ^ *begin) * fnvPrime) ^ *last; // Slightly modifies the algorithm to introduce a backward dependency

		return avalanche(hash);
	}

	return avalanche(hash);
}

} // namespace detail

} // namespace lsd
