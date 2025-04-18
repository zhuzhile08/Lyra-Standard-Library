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

// Concepts

template <class Ty> concept IteratableContainer = requires(Ty c1, Ty c2) {
	typename Ty::size_type;
	typename Ty::value_type;
	std::swap(c1, c2);
	std::begin(c1);
	std::end(c1);
};

template <class Ty> concept PointerType = std::is_pointer_v<Ty>;


namespace detail {


// Convert a size to an integer for safe member access

template <std::integral Integer> inline constexpr Integer sizeToIndex(Integer size) noexcept {
	return (size == 0) ? 0 : size - 1;
}


// Check allocator propagation

template <class Alloc> inline constexpr bool allocatorPropagationNecessary(const Alloc& a1, const Alloc& a2) {
	using traits_type = std::allocator_traits<Alloc>;

	if constexpr (traits_type::is_always_equal::value) return false;
	else return traits_type::propagate_on_container_move_assignment::value;
}

} // namespace detail


// Implicit cast

template <class Ty> [[nodiscard]] constexpr inline Ty implicitCast(std::type_identity_t<Ty> arg) noexcept(std::is_nothrow_constructible_v<Ty>) {
	return arg;
}
template <class Ty> [[deprecated]] [[nodiscard]] constexpr inline Ty implicit_cast(std::type_identity_t<Ty> arg) noexcept(std::is_nothrow_constructible_v<Ty>) {
	return arg;
}


// Value conditional

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


// String length

template <IteratableContainer Container>
[[nodiscard]] constexpr std::size_t stringLen(const Container& container) requires std::is_integral_v<typename Container::value_type> {
	std::size_t siz = 0;

	auto it = std::begin(container);
	while (it != std::end(container) && *it != '\0') {
		++siz;
		++it;
	}

	return siz;
}

template <std::integral Char>
[[nodiscard]] constexpr std::size_t stringLen(Char* string) {
	std::size_t siz = 0;

	while (*string != '\0') {
		++siz;
		++string;
	}

	return siz;
}


// Equal-to helper class

template <class Ty = void> class EqualTo {
public:
	constexpr bool operator()(const Ty& lhs, const Ty& rhs) const noexcept {
		return lhs == rhs;
	}

	template <class Other> constexpr bool operator()(const Ty& lhs, const Other& rhs) const noexcept 
		requires(requires(Ty a, Other b) { a == b; }) {
		return lhs == rhs;
	}
};

} // namespace lsd
