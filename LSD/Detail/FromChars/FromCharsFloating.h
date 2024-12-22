/*************************
 * @file FromCharsFloating.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief From-Chars functions for floating point numbers
 * 
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 *************************/

#include "FromCharsCore.h"
#include "FromCharsIntegral.h"
#include "ParseFloat.h"
#include "Tables.h"
#include "../../Iterators.h"
#include "../../Array.h"

#include <type_traits>
#include <system_error>
#include <limits>
#include <cctype>
#include <bit>

namespace lsd {

namespace detail {

// simplify parsed flaot

template <ContinuousIteratorType Iterator> constexpr bool simplifyFloat(
	FloatParseResult<Iterator>& result, std::int64_t mantissaMax, std::int64_t mantissaMin
) {
	if (result.fractional.empty()) {
		if (result.whole.remainingZeros > mantissaMax - mantissaMin) return false;
		result.exponent += result.whole.remainingZeros;

		if (result.exponent > mantissaMax || result.exponent < mantissaMin) return false;
		result.whole.end -= result.whole.remainingZeros;
	} else if (result.whole.empty()) {
		if (result.fractional.remainingZeros > mantissaMax - mantissaMin) return false;
		result.exponent -= result.fractional.remainingZeros;

		if (result.exponent > mantissaMax || result.exponent < mantissaMin) return false;
		result.fractional.begin += result.fractional.remainingZeros;
	}

	return true;
}


// handle special cases
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool handleTrivialCases(
	FloatParseResult<Iterator>& parseRes, Numerical& result
) {
	if (parseRes.mode == detail::FloatMode::infinity) {
		result = std::numeric_limits<Numerical>::infinity() * (parseRes.negative ? -1 : 1);
		
		return true;
	} else if (parseRes.mode == detail::FloatMode::notANumber) {
		result = std::numeric_limits<Numerical>::quiet_NaN() * (parseRes.negative ? -1 : 1);

		return true;
	} else if (parseRes.whole.empty() && parseRes.fractional.empty()) {
		result = (parseRes.exponent == 0) ? 1 : 0; 

		return true;
	} else if (parseRes.exponent == 0) {
		result = 1;

		return true;
	}

	return false;
}


// fast conversion
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool fastPath(
	const FloatParseResult<Iterator>& parseRes, Numerical& result, CharsFormat fmt
) {
	auto wholeLen = parseRes.whole.end - parseRes.whole.begin;
	auto fracLen = parseRes.fractional.end - parseRes.fractional.begin;

	auto e = parseRes.exponent - fracLen - 1;
	auto digitCount = wholeLen + fracLen;

	if (digitCount > 21 || e > parseRes.exponent)
		return false;

	auto base = (fmt == CharsFormat::hex) ? 16 : 10;

	std::uint64_t f = 0;
	if (!uncheckedFastUnsignedFromChars(parseRes.whole.begin, parseRes.whole.end, f, base)) return false;
	if (!uncheckedFastUnsignedFromChars(parseRes.fractional.begin, parseRes.fractional.end, f, base)) return false;

	if constexpr (std::is_same_v<Numerical, double>) {
		if (e >= 0)
			result = f * ((fmt == CharsFormat::hex) ? 1 << e : decDoublePowers[e]);
		else
			result = f / ((fmt == CharsFormat::hex) ? 1 << -e : decDoublePowers[-e]);
	} else {
		if (e >= 0)
			result = f * ((fmt == CharsFormat::hex) ? 1 << e : decFloatPowers[e]);
		else
			result = f / ((fmt == CharsFormat::hex) ? 1 << -e : decFloatPowers[-e]);
	}

	if (parseRes.negative) result *= -1;
	
	return true;
}


// bellerophon-derived floating point number parsing algorithm based on the dec2flt library in the rust core library
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool bellerophon(
	const FloatParseResult<Iterator>& parseRes, Numerical& result, CharsFormat fmt
) {
	///@todo

	if (parseRes.negative) result *= -1;
	
	return true;
}

} // namespace detail

template <class Numerical, IteratorType Iterator> constexpr FromCharsResult<Iterator> fromChars(
	Iterator begin, 
	Iterator end, 
	Numerical& result, 
	CharsFormat fmt = CharsFormat::general
) requires(
	(std::is_same_v<Numerical, float> || std::is_same_v<Numerical, double>) &&
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>
) {
	using namespace operators;

	constexpr static auto maxBinDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<52, 23>();
	constexpr static auto maxHexDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<14, 7>();
	constexpr static auto maxDecDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<17, 8>();

	constexpr static auto minExponent = ValueConditional<sizeof(Numerical) == 8, std::int64_t>::template get<-1022, -126>();
	constexpr static auto maxExponent = ValueConditional<sizeof(Numerical) == 8, std::int64_t>::template get<1023, 127>();


	detail::FloatParseResult<Iterator> parseRes { };

 	if (auto parseErr = detail::parseFloatingPoint(parseRes, begin, end, fmt); parseErr != std::errc { })
		return { begin, parseErr };

	if (detail::handleTrivialCases(parseRes, result))
		return { parseRes.last, std::errc { } };

	if (!detail::simplifyFloat(parseRes, minExponent, maxExponent))
		return { begin, std::errc::result_out_of_range };

	if (detail::fastPath(parseRes, result, fmt))
		return { parseRes.last, std::errc { } };

	if (detail::bellerophon(parseRes, result, fmt))
		return { parseRes.last, std::errc { } };

	return { begin, std::errc::invalid_argument };
}

} // namespace lsd
