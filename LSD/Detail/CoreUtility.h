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

#include <cstddef>

namespace lsd {

namespace detail {

// prime number utility

template <class Integer> inline constexpr bool isPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n == 2 || n == 3)
		return true;
	else if (n <= 1 || n % 2 == 0 || n % 3 == 0)
		return false;
	else for (Integer i = 5; i * i <= n; i += 6)
		if (n % i == 0 || n % (i + 2) == 0)
			return false;
	return true;
};

template <class Integer> inline constexpr Integer nextPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n % 2 == 0)
		--n;

	while (true) {
		n += 2;
		if (isPrime(n)) {
			return n;
		}
	}
};

template <class Integer> inline constexpr Integer lastPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n % 2 == 0)
		++n;

	while (true) {
		n -= 2;
		if (isPrime(n)) {
			return n;
		}
	}
};


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
	template <TTy TrueVal, FTy> [[nodiscard]] consteval static auto get() noexcept {
		return TrueVal;
	}
};
template <class TTy, class FTy> struct ValueConditional<false, TTy, FTy> {
	template <TTy, FTy FalseVal> [[nodiscard]] consteval static auto get() noexcept {
		return FalseVal;
	}
};


} // namespace lsd
