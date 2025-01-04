/*************************
 * @file ParseFloat.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Floating point number parsing
 * 
 * @brief Floating point number specification:
 * 
 * @brief float     := ['-'][body]
 * @brief body      := special | numerical
 * @brief special   := "INF" | "INFINITY" | "NAN" (case ignored)
 * @brief numerical := number["."number][exponent]
 * @brief exponent  := exp['-']number
 * @brief number    := <unsigned integer>
 * @brief exp       := 'P' | 'E' (case ignored)
 * 
 * @date 2024-12-21
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Core.h"
#include "Integral.h"
#include "../../Iterators.h"
#include "../../StringView.h"

#include <cstddef>
#include <type_traits>
#include <concepts>
#include <system_error>
#include <iterator>

namespace lsd {

namespace detail {

// structural implementations of the floating point spec

enum class FloatMode {
	regular,
	infinity,
	notANumber
};

template <ContinuousIteratorType Iterator> struct NumberView {
public:
	Iterator begin;
	Iterator end;

	std::size_t remainingZeros; // only one of either the trailing or leading zeros has to be stored, since the other will be automatically shortened, and only the other may be useful for simplification

	[[nodiscard]] constexpr bool empty() const noexcept {
		return begin == end;
	}
	[[nodiscard]] constexpr std::size_t size() const noexcept {
		return end - begin;
	}
};

template <ContinuousIteratorType Iterator> struct FloatParseResult {
public:
	using num_view = NumberView<Iterator>;

	bool negative = false;
	FloatMode mode = FloatMode::regular;

	num_view whole;
	num_view fractional;
	std::int64_t exponent = 0; // since the largest number a double can represent only has around 300 digits, this should be enougth

	Iterator last;
};


// number parsers

template <ContinuousIteratorType Iterator> constexpr bool parseHexNumber(
	NumberView<Iterator>& view, 
	Iterator& begin, 
	const Iterator& end,
	std::size_t& leadingZeros,
	std::size_t& trailingZeros
) {
	auto oldBegin = begin;

	// calculate leading zeros
	while (begin != end && *begin == '0') {
		++leadingZeros;
		++begin;
	}
	
	// parse number whilst calculating trailing zeros
	auto endFound = false; 
	while (!endFound && begin != end) {
		switch (*begin) {
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				++begin;
				trailingZeros = 0;

				continue;

			case '0':
				++begin;
				++trailingZeros;

				continue;

			default:
				endFound = true;
				break;
		}
	}

	if (oldBegin == begin && leadingZeros == 0) return false;

	view.begin = oldBegin;
	view.end = begin;

	return true;
}


template <ContinuousIteratorType Iterator> constexpr bool parseDecNumber(
	NumberView<Iterator>& view, 
	Iterator& begin, 
	const Iterator& end,
	std::size_t& leadingZeros,
	std::size_t& trailingZeros
) {
	auto oldBegin = begin;

	// calculate leading zeros
	while (begin != end && *begin == '0') {
		++leadingZeros;
		++begin;
	}
	
	// parse number whilst calculating trailing zeros
	auto endFound = false; 
	while (!endFound && begin != end) {
		switch (*begin) {
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
				++begin;
				trailingZeros = 0;

				continue;

			case '0':
				++begin;
				++trailingZeros;

				continue;

			default:
				endFound = true;
				break;
		}
	}


	if (oldBegin == begin && leadingZeros == 0) return false;
	
	view.begin = oldBegin;
	view.end = begin;
	
	return true;
}


// the parser itself

template <ContinuousIteratorType Iterator> constexpr std::errc parseFloatingPoint(
	FloatParseResult<Iterator>& result,
	Iterator begin,
	Iterator end,
	CharsFormat fmt
) {
	if (begin == end) return std::errc::invalid_argument;

	// parse sign
	if (*begin == '-') {
		++begin;
		result.negative = true;
	}
	
	if (begin == end) return std::errc::invalid_argument;


	// parse irregular modes
	switch (*begin) {
		case 'i': case 'I':
			if (detail::caselessStrNCmp(++begin, end, "nf", 2)) {
				result.mode = FloatMode::infinity;
				begin += 2;

				if (detail::caselessStrNCmp(begin, end, "inity", 5)) begin += 5;
				result.last = begin;

				return std::errc { };
			} 
			
			return std::errc::invalid_argument;

		case 'n': case 'N':
			if (detail::caselessStrNCmp(++begin, end, "an", 2)) {
				result.mode = FloatMode::notANumber;
				result.last = begin + 2;

				return std::errc { };
			}

			return std::errc::invalid_argument;
	}

	// the code now goes down 2 paths, depending on the format of the float
	if (fmt == CharsFormat::hex) { // hex float
		// parse whole part

		std::size_t wholeLeadingZeros = 0, wholeTrailingZeros = 0;
		auto emptyWhole = parseHexNumber(result.whole, begin, end, wholeLeadingZeros, wholeTrailingZeros); // don't throw an error, since this may be valid in some scenarios

		result.whole.begin += wholeLeadingZeros;
		result.whole.remainingZeros = wholeTrailingZeros;

		if (begin == end) {
			result.last = begin;

			return std::errc { };
		}


		// parse fractional part
		if (*begin == '.') {
			std::size_t fracLeadingZeros = 0, fracTrailingZeros = 0;

			if (!parseHexNumber(result.fractional, ++begin, end, fracLeadingZeros, fracTrailingZeros) && emptyWhole)
				return std::errc::invalid_argument; // here, there is an instant error, since there 

			result.fractional.end -= fracTrailingZeros;
			result.whole.remainingZeros = fracLeadingZeros;
			
			if (begin == end) {
				result.last = begin;

				return std::errc { };
			}
		}


		// parse exponent
		if (*begin == 'p' || *begin == 'P') {
			auto fcRes = fromChars(++begin, end, result.exponent, 16);

			if (fcRes.ec == std::errc { })
				result.last = fcRes.ptr;

			return fcRes.ec;
		} else if (emptyWhole)
			return std::errc::invalid_argument;
		
		return std::errc { };
	} else { // decimal float
		// parse whole part

		std::size_t wholeLeadingZeros = 0, wholeTrailingZeros = 0;
		auto emptyWhole = !parseDecNumber(result.whole, begin, end, wholeLeadingZeros, wholeTrailingZeros);

		result.whole.begin += wholeLeadingZeros;
		result.whole.remainingZeros = wholeTrailingZeros;

		if (begin == end) {
			result.last = begin;

			return std::errc { };
		}


		// parse fractional part
		if (*begin == '.') {
			std::size_t fracLeadingZeros = 0, fracTrailingZeros = 0;

			if (!parseDecNumber(result.fractional, ++begin, end, fracLeadingZeros, fracTrailingZeros) && emptyWhole)
				return std::errc::invalid_argument;

			result.fractional.end -= fracTrailingZeros;
			result.whole.remainingZeros = fracLeadingZeros;
			
			if (begin == end) {
				result.last = begin;

				return std::errc { };
			}
		}


		// parse exponent

		bool scientific = (fmt & CharsFormat::scientific) != 0;

		if ((*begin == 'e' || *begin == 'E') && scientific) {
			auto fcRes = fromChars(++begin, end, result.exponent, 10);

			if (fcRes.ec == std::errc { })
				result.last = fcRes.ptr;

			return fcRes.ec;
		} else if (((fmt & CharsFormat::fixed) == 0 && scientific) || emptyWhole)
			return std::errc::invalid_argument;

		result.last = begin;

		return std::errc { };
	}
}

} // namespace detail

} // namespace lsd
