/*************************
 * @file FromChars.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for the from_chars() function
 * 
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Iterators.h"

#include <type_traits>
#include <system_error>
#include <cctype>

namespace lsd {

// character sequence to numerics conversion

namespace detail {

// caseless strncmp for fromChars

template <class Iterator, typename std::enable_if_t<isIteratorValue<Iterator> && std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, int> = 0> 
bool caselessStrNCmp(Iterator lhs, Iterator lcrhs, std::size_t count) {
	for (; count > 0; count--, lhs++, lcrhs++) {
		if (std::towlower(*lhs) != *lcrhs) return false;
	} return true;
}

} // namespace detail

enum class CharsFormat {
	scientific = 0x1,
	fixed = 0x2,
	hex = 0x4,
	general = fixed | scientific
};

template <class Iterator, typename std::enable_if_t<isIteratorValue<Iterator>, int> = 0> struct FromCharsResult {
public:
	Iterator ptr;
	std::errc ec;

	constexpr explicit operator bool() const noexcept { 
		return ec == std::errc { }; 
	}
	friend constexpr bool operator==(const FromCharsResult&, const FromCharsResult&) = default;
};

template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_integral_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, int base = 10) {
	if (base > 36) return { begin, std::errc::invalid_argument };

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

	if (base >= 10) {
		const decltype(*begin) lowercaseLimit = ('a' + base);
		const decltype(*begin) uppercaseLimit = ('A' + base);

		for (; begin != end; begin++) {
            if (*begin >= '0'&& *begin <= '9') (res *= base) += *begin - '0';
			else if (*begin >= 'a') {
				if (*begin < lowercaseLimit) (res *= base) += 10 + *begin - 'a';
				else if (*begin >= 'A' && *begin < uppercaseLimit) (res *= base) += 10 + *begin - 'A';
				else break;
			} else break;

			++iterationCount;

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
            else prevRes = res;
		}
	} else {
		const decltype(*begin) numLimit = ('0' + base);

		for (; begin != end && *begin >= '0' && *begin < numLimit; begin++) {
			(res *= base) += *begin - '0';
			
			++iterationCount;

			if (prevRes > res) return { begin, std::errc::result_out_of_range };
            else prevRes = res;
		}
	}

	if (iterationCount != 0) result = res * sign;
	else return { beginCopy, std::errc::invalid_argument };

	return { begin, std::errc { } };
}

template <class Numerical, class Iterator, typename std::enable_if_t<
	std::is_floating_point_v<Numerical> && 
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, 
int> = 0> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, CharsFormat fmt = CharsFormat::general) {
	using namespace operators;

	auto beginCopy = begin;
	
	Numerical sign = 1;

	if (*begin == '-') {
		sign = -1;
		++begin;
	}

	switch (*begin) {
		case 'i': 
		case 'I':
			if ((end - ++begin) >= 2) if (detail::caselessStrNCmp(&*begin, "nf", 2)) {
				result = sign * std::numeric_limits<Numerical>::infinity();
				
				return { begin + 2, std::errc { } };
			}
			return { beginCopy, std::errc::invalid_argument };

			break;

		case 'n':
		case 'N':
			if ((end - ++begin) >= 2) if (detail::caselessStrNCmp(&*begin, "an", 2)) {
				result = sign * std::numeric_limits<Numerical>::infinity();
				
				return { begin + 2, std::errc { } };
			}
			return { beginCopy, std::errc::invalid_argument };

			break;
	}

	Numerical res = 0;
    Numerical prevRes = res;

	std::size_t iterationCount = 0;

	if ((fmt | CharsFormat::hex) == CharsFormat::hex) {
		//std::from_chars(nullptr, nullptr, double());
	} else {
	
	}

	if (iterationCount != 0) result = res * sign;
	else return { beginCopy, std::errc::invalid_argument };

	return { begin, std::errc { } };
}

} // namespace lsd
