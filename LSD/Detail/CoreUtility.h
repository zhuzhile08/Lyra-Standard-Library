/*************************
 * @file CoreUtility.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core utility headers for the library with minimal dependencies
 * 
 * @date 2024-06-23
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../MathExt.h"

#include <cstddef>
#include <type_traits>
#include <concepts>

#include <memory>

namespace lsd {

// concepts

template <class Ty> concept IteratableContainer = requires(Ty c1, Ty c2) {
	typename Ty::size_type;
	typename Ty::value_type;
	c1.swap(c2);
	c1.begin();
	c1.end();
};


namespace detail {

// hash map utility

inline constexpr std::size_t hashmapBucketSizeCheck(std::size_t requested, std::size_t required) noexcept {
	return (requested < required) ? nextPrime(required) : nextPrime(requested);
}


// convert a size to an integer for safe member access

template <class Integer> inline constexpr Integer sizeToIndex(Integer size) noexcept requires std::is_integral_v<Integer> {
	return (size == 0) ? 0 : size - 1;
}


// check allocator propagation

template <class Alloc> inline constexpr bool allocatorPropagationNecessary(const Alloc& a1, const Alloc& a2) {
	using traits_type = std::allocator_traits<Alloc>;

	if constexpr (traits_type::is_always_equal::value) return false;
	else return traits_type::propagate_on_container_move_assignment::value && !(a1 == a2);
}

} // namespace detail


// implicit cast

template <class Ty> [[nodiscard]] constexpr inline Ty implicitCast(std::type_identity_t<Ty> arg) noexcept(std::is_nothrow_constructible_v<Ty>) {
	return arg;
}
template <class Ty> [[deprecated]] [[nodiscard]] constexpr inline Ty implicit_cast(std::type_identity_t<Ty> arg) noexcept(std::is_nothrow_constructible_v<Ty>) {
	return arg;
}


// value conditional

template <bool Condition, class TTy, class FTy = TTy> struct ValueConditional;
template <class TTy, class FTy> struct ValueConditional<true, TTy, FTy> {
	template <TTy TrueVal, FTy> [[nodiscard]] consteval static decltype(auto) get() noexcept {
		return TrueVal;
	}
};
template <class TTy, class FTy> struct ValueConditional<false, TTy, FTy> {
	template <TTy, FTy FalseVal> [[nodiscard]] consteval static decltype(auto) get() noexcept {
		return FalseVal;
	}
};

} // namespace lsd
