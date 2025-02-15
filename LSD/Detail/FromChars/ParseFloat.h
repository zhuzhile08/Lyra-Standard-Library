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

// Structural implementation of the floating point spec

template <ContinuousIteratorType Iterator> struct FloatParseResult {
public:
	bool negative = false;

	std::size_t wholeSize = 0;
	std::size_t fracSize = 0;

	std::uint64_t mantissa = 0;
	std::int64_t exponent = 0; // Since the largest number a double can represent only has around 300 digits, this should be enougth

	bool fastPathAvailable = true;

	Iterator last;
};


// From chars integral specialized for floating point parsing

template <class Iterator>
constexpr FromCharsResult<Iterator> unsignedFromCharsBase10(Iterator begin, Iterator end, std::uint64_t& result, std::size_t* parsedDigits)
requires (
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type> 
) {
	constexpr std::uint64_t maxVal = std::numeric_limits<std::uint64_t>::max();
	
	constexpr std::uint64_t maxValOverBase = maxVal / 10;
	constexpr std::uint64_t maxLastDigit = maxVal % 10;

	std::errc ec { };
	std::size_t iterationCount = 0;

	std::uint8_t n = 0; 
	while (begin != end && *begin >= '0' && *begin <= '9') {
		n = *begin - '0';

		if (result > maxValOverBase || (result == maxValOverBase && n > maxLastDigit)) {
			ec = std::errc::result_out_of_range;

			break;
		}

		result = result * 10 + n;

		++begin;
		++iterationCount;
	}

	if (iterationCount != 0) {
		if (parsedDigits) *parsedDigits = iterationCount;

		return { begin, ec };
	} else return { begin, std::errc::invalid_argument };
}


// The parser itself

template <ContinuousIteratorType Iterator, class Floating> constexpr std::errc parseFloatingPoint(
	FloatParseResult<Iterator>& result,
	Iterator begin,
	Iterator end,
	CharsFormat fmt,
	Floating& fres
) {
	static constexpr std::uint64_t maxVal = std::numeric_limits<std::uint64_t>::max();

	if (begin == end) return std::errc::invalid_argument;

	// Parse sign
	if (*begin == '-') {
		if (++begin == end) return std::errc::invalid_argument;

		result.negative = true;
	}


	// Parse irregular modes
	switch (*begin) {
		case 'i': case 'I':
			if (detail::caselessStrNCmp(++begin, end, "nf", 2)) {
				fres = std::numeric_limits<Floating>::infinity() * (result.negative ? -1 : 1);
				begin += 2;

				if (detail::caselessStrNCmp(begin, end, "inity", 5)) begin += 5;
				result.last = begin;

				return std::errc::operation_canceled;
			} 
			
			return std::errc::invalid_argument;

		case 'n': case 'N':
			if (detail::caselessStrNCmp(++begin, end, "an", 2)) {
				fres = std::numeric_limits<Floating>::quiet_NaN() * (result.negative ? -1 : 1);
				result.last = begin + 2;

				return std::errc::operation_canceled;
			}

			return std::errc::invalid_argument;
	}


	// The code now goes down 2 paths, depending on the format of the float
	if (fmt == CharsFormat::hex) { // Hex float
		/*
		// Parse whole part

		auto fcRes = fromChars(begin, end, result.mantissa, 10, &result.wholeSize);
		if (fcRes.ptr == begin || fcRes.ec == std::errc::invalid_argument) return std::errc::invalid_argument;

		begin = fcRes.ptr;

		if (begin == end) {
			result.last = begin;

			return std::errc { };
		}



		// Parse fractional part
		if (*begin == '.') {
			if (!parseHexNumber(result.mantissa, result.fractionalSize, begin, end))
				return std::errc::invalid_argument;
		}


		// Parse exponent
		if (*begin == 'p' || *begin == 'P') {
			auto fcRes = fromChars(++begin, end, result.exponent, 16);

			if (fcRes.ec == std::errc { })
				result.last = fcRes.ptr;

			return fcRes.ec;
		} else if (emptyWhole)
			return std::errc::invalid_argument;
		
		return std::errc { };
		*/
	} else { // Decimal float
		// Parse whole part

		auto wholeFcRes = unsignedFromCharsBase10(begin, end, result.mantissa, &result.wholeSize);

		if (wholeFcRes.ptr == end) { // It can only be end if something was parsed, which is why this is ok here
			result.last = wholeFcRes.ptr;

			return std::errc { };
		}

		begin = wholeFcRes.ptr;


		if (wholeFcRes.ec == std::errc::result_out_of_range) { // Just skips the entire rest of the number until the exponent
			result.fastPathAvailable = false;

			while (begin != end && *begin >= '0' && *begin <= '9') {
				++begin;
				++result.exponent; // Small hack, combined with the funny part with the exponent parsing
			}

			if (begin != end && *begin == '.') ++begin;

			while (begin != end && *begin >= '0' && *begin <= '9') ++begin;

			if (begin == end) {
				result.last = begin;

				return std::errc { };
			}
		} else if (*begin == '.') { // Parse fractional part
			auto fracFcRes = unsignedFromCharsBase10(++begin, end, result.mantissa, &result.fracSize);
			if (fracFcRes.ec == std::errc::invalid_argument && wholeFcRes.ec == std::errc::invalid_argument)
				return std::errc::invalid_argument;

			if (result.mantissa == 0) {
				fres = 0; // Special exit case, where both whole and fractional were all zeros

				return std::errc::operation_canceled;
			}

			if (fracFcRes.ptr == end) {
				result.last = fracFcRes.ptr;

				return std::errc { };
			}

			begin = fracFcRes.ptr;

			// Skip until exponent
			if (fracFcRes.ec == std::errc::result_out_of_range) {
				result.fastPathAvailable = false;
				++begin;

				while (begin != end && *begin >= '0' && *begin <= '9') ++begin;

				if (begin == end) {
					result.last = begin;

					return std::errc { };
				}
			}
		} else if (wholeFcRes.ec == std::errc::invalid_argument) // Special case where neither the whole or floating part was present
			result.mantissa = 1;


		// Parse exponent

		bool scientific = (fmt & CharsFormat::scientific) != 0;

		if ((*begin == 'e' || *begin == 'E') && scientific) {
			auto expfac = result.exponent;
			auto fcRes = fromChars(++begin, end, result.exponent);
			result.exponent += expfac;

			if (fcRes.ec != std::errc { })
				return fcRes.ec;

			result.last = fcRes.ptr;
			return std::errc { };
		} else if ((fmt & CharsFormat::fixed) == 0 && scientific)
			return std::errc::invalid_argument;

		result.last = begin;
		return std::errc { };
	}
}

} // namespace detail

} // namespace lsd
