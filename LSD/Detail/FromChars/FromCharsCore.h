/*************************
 * @file FromCharsCore.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core utility and structures for the From-Chars library
 * 
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../../Iterators.h"

#include <type_traits>
#include <system_error>
#include <cctype>

namespace lsd {

enum class CharsFormat {
	scientific = 0x1,
	fixed = 0x2,
	hex = 0x4,
	general = fixed | scientific
};


template <ContinuousIteratorType Iterator> struct FromCharsResult {
public:
	Iterator ptr;
	std::errc ec;

	constexpr explicit operator bool() const noexcept { 
		return ec == std::errc { }; 
	}
	friend constexpr bool operator==(const FromCharsResult&, const FromCharsResult&) = default;
};


namespace detail {

// caseless strncmp
template <ContinuousIteratorType Iterator, ContinuousIteratorType Comparison> 
constexpr bool caselessStrNCmp(Iterator begin, Iterator end, Comparison cmp, std::size_t count) requires(
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type> &&
	std::is_integral_v<typename std::iterator_traits<Comparison>::value_type>
) {
	for (; count > 0 && begin != end; count--, begin++, cmp++) {
		auto c = *begin;
		if (c < 'a') c += 'a' - 'A';

		if (c != *cmp) return false;
	}

	if (count > 0) return false;
	else return true;
}


// digit validity checks

constexpr inline std::size_t isHexDigit(int digit) noexcept {
	return (digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'F') || (digit >= 'a' && digit <= 'f');
}

constexpr inline std::size_t isDecDigit(int digit) noexcept {
	return digit >= '0' && digit <= '9';
}

} // namespace detail

} // namespace lsd
