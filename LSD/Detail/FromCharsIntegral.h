/*************************
 * @file FromCharsIntegral.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief From-Chars functions for integral types
 * 
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "FromCharsCore.h"

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
		const decltype(*begin) uppercaseLimit = ('A' + base - 10);
		const decltype(*begin) lowercaseLimit = ('a' + base - 10);

		for (; begin != end; begin++, iterationCount++) {
            if (*begin >= '0' && *begin <= '9') (res *= base) += *begin - '0';
			else if (*begin >= 'A') {
				if (*begin < uppercaseLimit) (res *= base) += 10 + *begin - 'A';
				else if (*begin >= 'a' && *begin < lowercaseLimit) (res *= base) += 10 + *begin - 'a';
				else break;
			} else break;

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
            else prevRes = res;
		}
	} else {
		const decltype(*begin) numLimit = ('0' + base);

		for (; begin != end && *begin >= '0' && *begin < numLimit; begin++, iterationCount++) {
			(res *= base) += *begin - '0';

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
            else prevRes = res;
		}
	}

	if (iterationCount != 0) {
        result = res * sign;
	    return { begin, std::errc { } };
    } else return { beginCopy, std::errc::invalid_argument };
}

} // namespace lsd
