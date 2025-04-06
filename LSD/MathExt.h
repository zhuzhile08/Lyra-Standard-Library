/************************
 * @file MathExt.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Extensions for standard math functions
 * 
 * @date 2024-10-29
 * @copyright Copyright (c) 2024
 ************************/

#pragma once

#include <type_traits>
#include <concepts>

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace lsd {

namespace detail {

// number digit counting utility

template <std::integral Type> inline constexpr std::size_t u32LenDec(Type x) {
	if (x >= 100000) {
		if (x >= 10000000) {
			if (x >= 100000000) {
				if (x >= 1000000000) return 10;
				else return 9;
			} else return 8;
		} if (x >= 1000000) return 7;
		else return 6;
	} else if (x >= 100) {
		if (x >= 1000) {
			if (x >= 10000) return 5;
			else return 4;
		} else return 3;
	} else if (x >= 10) return 2;
	else return 1;
}

template <std::integral Type> inline constexpr std::size_t i64LenDec(Type x) {
	if (x >= 10000000000) {
		if (x >= 100000000000000) {
			if (x >= 10000000000000000) {
				if (x >= 100000000000000000) {
					if (x >= 1000000000000000000) return 19;
					else return 18;
				} else return 17;
			} else if (x >= 1000000000000000) return 16;
			else return 15;
		} else if (x >= 1000000000000) {
			if (x >= 10000000000000) return 14;
			else return 13;
		} else if (x >= 100000000000) return 12;
		else return 11;
	} else return u32LenDec<Type>(x);
}

} // namespace detail


// count digits of a number

template <std::integral Type> inline constexpr std::size_t decNumLen(Type value) {
	if constexpr (sizeof(Type) == 4) {
		if (value < 0) value *= -1;
		return detail::u32LenDec(value);
	} else if constexpr (sizeof(Type) == 8) {
		if constexpr (std::is_unsigned_v<Type>) {
			if (value >= 10000000000000000000ULL) return 20;
			else return detail::i64LenDec(value);
		} else {
			if (value < 0) value *= -1;
			return detail::i64LenDec(value);
		}
	} else {
		if (value < 10) return 1;

		std::size_t digits = 1;
		for (; value >= 10; value /= 10, digits++)
		;
		return digits;
	}
}

template <std::size_t Base, class Type> inline constexpr std::size_t numLen(Type value) requires std::is_arithmetic_v<Type> {
	if (value < Base) return 1;

	std::size_t digits = 1;
	for (; value >= Base; value /= Base, digits++)
	;
	return digits;
}
template <class Type> inline constexpr std::size_t numLen(Type value, std::size_t base) requires std::is_arithmetic_v<Type> {
	if (value < base) return 1;

	std::size_t digits = 1;
	for (; value >= base; value /= base, digits++)
	;
	return digits;
}


// log base n log x

template <std::int64_t Base, class Floating> inline constexpr Floating logn(Floating x) {
	return std::log(x) / std::log(Base);
}
template <class Floating> inline constexpr Floating logn(Floating base, Floating x) {
	return std::log(x) / std::log(base);
}

} // namespace lsd
