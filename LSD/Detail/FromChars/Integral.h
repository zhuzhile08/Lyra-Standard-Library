/*************************
 * @file Integral.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief From-Chars functions for integral types
 * 
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Core.h"

namespace lsd {

template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_integral_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, int base = 10) {
	if (base > 36 || begin == end) return { begin, std::errc::invalid_argument };

	auto beginCopy = begin;

	Numerical sign = 1;

	if (*begin == '-') {
		if constexpr (std::is_signed_v<Numerical>) sign = -1;
		else return { begin, std::errc::invalid_argument };

		++begin;
	}

	Numerical res = 0;
	Numerical prevRes = res;

	std::size_t iterationCount = 0;

	if (base > 10) {
		const std::remove_cvref_t<decltype(*begin)> uppercaseLimit = ('A' + base - 10);
		const std::remove_cvref_t<decltype(*begin)> lowercaseLimit = ('a' + base - 10);

		for (; begin != end; begin++, iterationCount++) {
			res *= base;
			if (res / base != prevRes) return { begin, std::errc::result_out_of_range };
			prevRes = res;

			if (*begin >= '0' && *begin <= '9') res += *begin - '0';
			else if (*begin >= 'A') {
				if (*begin < uppercaseLimit) res += 10 + *begin - 'A';
				else if (*begin >= 'a' && *begin < lowercaseLimit) res += 10 + *begin - 'a';
				else break;
			} else break;

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
			else prevRes = res;
		}
	} else {
		const std::remove_cvref_t<decltype(*begin)> numLimit = ('0' + base);

		for (; begin != end && *begin >= '0' && *begin < numLimit; begin++, iterationCount++) {
			res *= base;
			if (res / base != prevRes) return { begin, std::errc::result_out_of_range };
			prevRes = res;

			res += *begin - '0';

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
			else prevRes = res;
		}
	}

	if (iterationCount != 0) {
		result = res * sign;
		return { begin, std::errc { } };
	} else return { beginCopy, std::errc::invalid_argument };
}


namespace detail {

// Fast and unsafe versions of fromChars, do NOT use this under any normal circumstances

template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_unsigned_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr void uncheckedFastUnsignedBase10FromChars(Iterator begin, Iterator end, Numerical& result) {
	for (; begin != end; begin++)
		result = result * 10 + *begin - '0';
}

template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_unsigned_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr bool uncheckedFastUnsignedBase16FromChars(Iterator begin, Iterator end, Numerical& result) {
	Numerical prev = result;

	for (; begin != end; begin++) {
		if (*begin <= 'A') (result *= 16) += *begin - '0';
		else if (*begin < 'a') (result *= 16) += 10 + *begin - 'A';
		else (result *= 16) += 10 + *begin - 'a';

		if (result < prev) return false;
		prev = result;
	}

	return true;
}


// Same principle as above, but parses until the next digit overflows
template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_unsigned_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr bool uncheckedOverflowBoundBase10FastUnsignedFromChars(Iterator begin, Iterator end, Numerical& result, std::size_t& digitsParsed) {
	static constexpr Numerical maxVal = std::numeric_limits<Numerical>::max();
	static constexpr Numerical maxValOverTen = maxVal / 10;
	static constexpr Numerical maxLastDigit = maxVal % 10;

	for (; begin != end; begin++) {
		auto n = *begin - '0';

		if (result > maxValOverTen || (result == maxValOverTen && n > maxLastDigit))
			return false;

		result = result * 10 + n;
		++digitsParsed;
	}

	return true;
}

} // namespace detail

} // namespace lsd
