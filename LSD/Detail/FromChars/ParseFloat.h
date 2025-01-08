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

	constexpr char zeroC = '0';
	constexpr char nineC = '9' + 1;

	std::errc ec { };
	std::size_t iterationCount = 0;

	for (std::uint8_t n = 0; begin != end; begin++, iterationCount++) {
		n = *begin - '0';

		if (result > maxValOverBase || (result == maxValOverBase && n > maxLastDigit)) {
			ec = std::errc::result_out_of_range;

			break;
		}

		result = result * 10 + n;
	}

	if (iterationCount != 0) {
		if (parsedDigits) *parsedDigits = iterationCount;

		return { begin, ec };
	} else return { begin, std::errc::invalid_argument };
}


// structural implementation of the floating point spec

template <ContinuousIteratorType Iterator> struct FloatParseResult {
public:
	bool negative = false;

	std::size_t wholeSize = 0;
	std::size_t fracSize = 0;

	std::uint64_t mantissa = 0;
	std::int64_t exponent = 0; // since the largest number a double can represent only has around 300 digits, this should be enougth

	bool fastPathAvailable = true;

	Iterator last;
};


// the parser itself

template <ContinuousIteratorType Iterator, class Floating> constexpr std::errc parseFloatingPoint(
	FloatParseResult<Iterator>& result,
	Iterator begin,
	Iterator end,
	CharsFormat fmt,
	Floating& fres
) {
	static constexpr std::uint64_t maxVal = std::numeric_limits<std::uint64_t>::max();

	if (begin == end) return std::errc::invalid_argument;

	// parse sign
	if (*begin == '-') {
		if (++begin == end) return std::errc::invalid_argument;

		result.negative = true;
	}


	// parse irregular modes
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


	// the code now goes down 2 paths, depending on the format of the float
	if (fmt == CharsFormat::hex) { // hex float
		/*
		// parse whole part

		auto fcRes = fromChars(begin, end, result.mantissa, 10, &result.wholeSize);
		if (fcRes.ptr == begin || fcRes.ec == std::errc::invalid_argument) return std::errc::invalid_argument;

		begin = fcRes.ptr;

		if (begin == end) {
			result.last = begin;

			return std::errc { };
		}



		// parse fractional part
		if (*begin == '.') {
			if (!parseHexNumber(result.mantissa, result.fractionalSize, begin, end))
				return std::errc::invalid_argument;
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
		*/
	} else { // decimal float
		// parse whole part

		auto wholeFcRes = unsignedFromCharsBase10(begin, end, result.mantissa, &result.wholeSize);

		if (wholeFcRes.ptr == end) { // it can only be end if something was parsed, which is why this is ok here
			result.last = wholeFcRes.ptr;

			return std::errc { };
		}

		begin = wholeFcRes.ptr;


		if (wholeFcRes.ec == std::errc::result_out_of_range) { // just skips the entire rest of the number until the exponent
			result.fastPathAvailable = false;

			while (begin != end) {
				switch (*begin) {
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':
						++begin;
						continue;
				}

				break;
			}

			if (begin == end) {
				result.last = begin;

				return std::errc { };
			}
		} else if (*begin == '.') { // parse fractional part
			auto fracFcRes = unsignedFromCharsBase10(++begin, end, result.mantissa, &result.fracSize);
			if (fracFcRes.ec == std::errc::invalid_argument && wholeFcRes.ec == std::errc::invalid_argument)
				return std::errc::invalid_argument;

			if (result.mantissa == 0) {
				fres = 0; // special exit case, where both whole and fractional were all zeros

				return std::errc::operation_canceled;
			}

			if (fracFcRes.ptr == end) {
				result.last = fracFcRes.ptr;

				return std::errc { };
			}

			begin = wholeFcRes.ptr;

			// skip until exponent
			if (fracFcRes.ec == std::errc::result_out_of_range) {
				result.fastPathAvailable = false;
				++begin;

				while (begin != end) {
					switch (*begin) {
						case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
							++begin;
							continue;
					}

					break;
				}

				if (begin == end) {
					result.last = begin;

					return std::errc { };
				}
			}
		} else if (wholeFcRes.ec == std::errc::invalid_argument) // special case where neither the whole or floating part was present
			result.mantissa = 1;


		// parse exponent

		bool scientific = (fmt & CharsFormat::scientific) != 0;

		if ((*begin == 'e' || *begin == 'E') && scientific) {
			auto fcRes = fromChars(++begin, end, result.exponent, 10);

			if (fcRes.ptr == end)
				result.last = fcRes.ptr;

			return fcRes.ec;
		} else if ((fmt & CharsFormat::fixed) == 0 && scientific)
			return std::errc::invalid_argument;

		result.last = begin;

		return std::errc { };
	}
}

} // namespace detail

} // namespace lsd
