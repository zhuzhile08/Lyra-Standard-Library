/*************************
 * @file Hash.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Common hash function implementations
 * 
 * @date 2024-03-02
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Operators.h"
#include "Detail/CoreUtility.h"

#include <type_traits>
#include <concepts>
#include <functional>
#include <cstdint>
#include <functional>

namespace lsd {

// Forward declarations and metaprogramming utilites

template <class> struct Hash;
template <class> struct FancyHash;


template <class, class = void> class HashRequiresPostMix : public std::false_type { };
template <class Ty> class HashRequiresPostMix<
	Ty, std::void_t<typename Ty::requires_post_mixing>
> : public std::is_same<typename Ty::requires_post_mixing, std::true_type> { };

template <class Ty> inline constexpr bool hashRequiresPostMixValue = HashRequiresPostMix<Ty>::value;



// Fallback

template <class Ty> struct Hash {
	std::size_t operator()(const Ty& v) const noexcept {
		return std::hash<Ty>()(v);
	}
};


// Integer hashes

namespace detail {

template <class Ty> concept ByteInt = std::is_integral_v<Ty> && sizeof(Ty) == 1;
template <class Ty> concept StandardInt = std::is_integral_v<Ty> && sizeof(Ty) > 1 && sizeof(Ty) <= sizeof(std::size_t);
template <class Ty> concept BigInt = std::is_integral_v<Ty> && sizeof(Ty) > sizeof(std::size_t);

} // namespace detail

template <detail::ByteInt Integral> struct Hash<Integral> {
	constexpr std::size_t operator()(Integral i) const noexcept {
		static constexpr auto size = sizeof(Integral);

		using size32 = std::conditional_t<size == 8, std::size_t, std::size_t>;
		using size64 = std::conditional_t<size == 8, std::size_t, std::uint64_t>;
		using value_conditional = ValueConditional<size == 8, size64, size32>;

		static constexpr std::size_t repeatedBitPattern = value_conditional::template get<
			size64 { 0x0101010101010101 }, size32 { 0x01010101 }>();

		return i * repeatedBitPattern;
	}
};

template <detail::StandardInt Integral> struct Hash<Integral> {
	using requires_post_mixing = std::true_type;

	constexpr std::size_t operator()(Integral i) const noexcept {
		return static_cast<std::size_t>(i);
	}
};

template <detail::BigInt Integral> struct Hash<Integral> {
	constexpr std::size_t operator()(Integral i) const noexcept {
		// It is hereon assumed that std::size_t is 32 bits, and the input is 64 bits
		static_assert(sizeof(std::size_t) == 4 && sizeof(Integral) == 8, "lsd::Hash<Integral>()::operator(): When std::size_t is smaller than the input, it has to be 32 bits wide and the input has to be 64 wide!");

		i ^= i >> 33;
		i *= 0xFF51ADF7ED558CCD;
		return static_cast<std::size_t>(i ^ i >> 33);
	}
};


template <EnumType Enum> struct Hash<Enum> {
	using underlying = std::underlying_type_t<Enum>;
	using underlying_hash = Hash<underlying>;
	using requires_post_mixing = HashRequiresPostMix<underlying_hash>;
	
	constexpr std::size_t operator()(Enum e) const noexcept {
		return underlying_hash()(static_cast<underlying>(e));
	}
};

template <PointerType Pointer> struct Hash<Pointer> {
	using underlying_hash = Hash<std::uintptr_t>;
	using requires_post_mixing = HashRequiresPostMix<underlying_hash>;

	std::size_t operator()(Pointer p) const noexcept {
		return underlying_hash()(reinterpret_cast<std::uintptr_t>(p));
	}
};

template <> struct Hash<std::nullptr_t> {
	using underlying_hash = Hash<std::uintptr_t>;
	using requires_post_mixing = HashRequiresPostMix<underlying_hash>;

	consteval std::size_t operator()(std::nullptr_t) const noexcept {
		return underlying_hash()(0u);
	}
};

} // namespace lsd


// Custom hasher macros for hash tables

#define LSD_CUSTOM_HASHER(name, type, hashType, hasher, toHashType)\
class name {\
public:\
	constexpr std::size_t operator()(type ty) const noexcept {\
		return hasher((ty)toHashType);\
	}\
	constexpr std::size_t operator()(hashType hash) const noexcept {\
		return hasher(hash);\
	}\
};

#define LSD_CUSTOM_EQUAL(name, type, hashType, toHashType)\
class name {\
public:\
	constexpr bool operator()(type first, type second) const noexcept {\
		return (first)toHashType == (second)toHashType;\
	}\
	constexpr bool operator()(type first, hashType second) const noexcept {\
		return (first)toHashType == second;\
	}\
	constexpr bool operator()(hashType first, type second) const noexcept {\
		return first == (second)toHashType;\
	}\
	constexpr bool operator()(hashType first, hashType second) const noexcept {\
		return first == second;\
	}\
};
