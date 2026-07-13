/*************************
 * @file CoreUtility.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core utility headers for the library with minimal dependencies
 * 
 * @date 2024-06-23
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include <cstddef>
#include <type_traits>
#include <concepts>
#include <iterator>


namespace lsd {

// Concepts

template <class Ty> concept IteratableContainer = requires(Ty c1, Ty c2) {
	typename Ty::size_type;
	typename Ty::value_type;
	c1.swap(c2);
	std::begin(c1);
	std::end(c1);
};

template <class Ty> concept PointerType = std::is_pointer_v<Ty>;


namespace detail {


// Convert a size to an integer for safe member access

template <std::integral Integer> inline constexpr Integer sizeToIndex(Integer size) noexcept {
	return (size == 0) ? 0 : size - 1;
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


// Compile time min and max

template <std::three_way_comparable Ty> struct ValueComparison {
	template <Ty a, Ty b> [[nodiscard]] consteval static decltype(auto) min() noexcept {
		if constexpr (a > b)
			return b;
		return a;
	}
	template <Ty a, Ty b> [[nodiscard]] consteval static decltype(auto) max() noexcept {
		if constexpr (a > b)
			return a;
		return b;
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
