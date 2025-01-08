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

// Default, standard compatible from chars
template <class Numerical, class Iterator> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, int base = 10)
requires (
	std::is_integral_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type> 
) {
	constexpr Numerical maxVal = std::numeric_limits<Numerical>::max();

	if (base < 2 || base > 36 || begin == end) return { begin, std::errc::invalid_argument };

	auto beginCopy = begin;

	Numerical sign = 1;

	if (*begin == '-') {
		if constexpr (std::is_signed_v<Numerical>) sign = -1;
		else return { begin, std::errc::invalid_argument };

		++begin;
	}

	const Numerical maxValOverBase = maxVal / base;
	const Numerical maxLastDigit = maxVal % base;

	Numerical res = 0;

	std::size_t iterationCount = 0;

	if (base > 10) {
		const std::remove_cvref_t<decltype(*begin)> uppercaseLimit = ('A' + base - 10);
		const std::remove_cvref_t<decltype(*begin)> lowercaseLimit = ('a' + base - 10);

		for (std::uint8_t n = 0; begin != end; begin++, iterationCount++) {
			if (*begin >= '0' && *begin <= '9') n = *begin - '0';
			else if (*begin >= 'A') {
				if (*begin < uppercaseLimit) n = 10 + *begin - 'A';
				else if (*begin >= 'a' && *begin < lowercaseLimit) n = 10 + *begin - 'a';
				else break;
			} else break;

			if (res > maxValOverBase || (res == maxValOverBase && n > maxLastDigit))
				return { beginCopy, std::errc::result_out_of_range };

			res = res * base + n;
		}
	} else {
		const std::remove_cvref_t<decltype(*begin)> numLimit = ('0' + base);

		for (std::uint8_t n = 0; begin != end && *begin >= '0' && *begin < numLimit; begin++, iterationCount++) {
			n = *begin - '0';

			if (res > maxValOverBase || (res == maxValOverBase && n > maxLastDigit))
				return { beginCopy, std::errc::result_out_of_range };

			res = res * base + n;
		}
	}

	if (iterationCount != 0) {
		result = res * sign;
		return { begin, std::errc { } };
	} else return { beginCopy, std::errc::invalid_argument };
}


// Extended from chars
template <class Numerical, class Iterator> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, std::size_t* parsedDigits, int base = 10)
requires (
	std::is_integral_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type> 
) {
	constexpr Numerical maxVal = std::numeric_limits<Numerical>::max();

	if (base < 2 || base > 36 || begin == end) return { begin, std::errc::invalid_argument };

	auto beginCopy = begin;

	Numerical sign = 1;

	if (*begin == '-') {
		if constexpr (std::is_signed_v<Numerical>) sign = -1;
		else return { begin, std::errc::invalid_argument };

		++begin;
	}

	const Numerical maxValOverBase = maxVal / base;
	const Numerical maxLastDigit = maxVal % base;

	std::errc ec { };
	std::size_t iterationCount = 0;

	if (base > 10) {
		const std::remove_cvref_t<decltype(*begin)> uppercaseLimit = ('A' + base - 10);
		const std::remove_cvref_t<decltype(*begin)> lowercaseLimit = ('a' + base - 10);

		for (std::uint8_t n = 0; begin != end; begin++, iterationCount++) {
			if (*begin >= '0' && *begin <= '9') n = *begin - '0';
			else if (*begin >= 'A') {
				if (*begin < uppercaseLimit) n = 10 + *begin - 'A';
				else if (*begin >= 'a' && *begin < lowercaseLimit) n = 10 + *begin - 'a';
				else break;
			} else break;

			if (result > maxValOverBase || (result == maxValOverBase && n > maxLastDigit)) {
				ec = std::errc::result_out_of_range;

				break;
			}

			result = result * base + n;
		}
	} else {
		const std::remove_cvref_t<decltype(*begin)> numLimit = ('0' + base);

		for (std::uint8_t n = 0; begin != end && *begin >= '0' && *begin < numLimit; begin++, iterationCount++) {
			n = *begin - '0';

			if (result > maxValOverBase || (result == maxValOverBase && n > maxLastDigit)) {
				ec = std::errc::result_out_of_range;

				break;
			}

			result = result * base + n;
		}
	}

	if (iterationCount != 0) {
		result *= sign;
		if (parsedDigits) *parsedDigits = iterationCount;

		return { begin, ec };
	} else return { beginCopy, std::errc::invalid_argument };
}

} // namespace lsd
