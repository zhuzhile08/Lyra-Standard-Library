/*************************
 * @file Floating.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief From-Chars functions for floating point numbers
 * 
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 *************************/

#include "Core.h"
#include "Integral.h"
#include "ParseFloat.h"
#include "Tables.h"

#include <type_traits>
#include <system_error>
#include <cctype>
#include <bit>

namespace lsd {

namespace detail {

// some numerical details about each floating point type

template <class Numerical> class FloatingPointInfo;

template <> class FloatingPointInfo<float> {
public:
	using value_type = float;
	using uint_type = std::uint32_t;

	consteval static auto expMin() noexcept {
		return -126;
	}
	consteval static auto expMax() noexcept {
		return 127;
	}
	consteval static auto expBias() noexcept {
		return 127;
	}
	consteval static auto mantShift() noexcept {
		return 23;
	}
};

template <> class FloatingPointInfo<double> {
public:
	using value_type = float;
	using uint_type = std::uint64_t;

	consteval static auto expMin() noexcept {
		return -1022;
	}
	consteval static auto expMax() noexcept {
		return 1023;
	}
	consteval static auto expBias() noexcept {
		return 1023;
	}
	consteval static auto mantShift() noexcept {
		return 52;
	}
};


// simplify parsed float
template <class Numerical, ContinuousIteratorType Iterator> constexpr bool simplifyFloat(FloatParseResult<Iterator>& result) {
	using fp_info = FloatingPointInfo<Numerical>;

	if (result.fractional.empty()) {
		if (result.whole.remainingZeros > fp_info::expMax() - fp_info::expMin()) return false;
		result.exponent += result.whole.remainingZeros;

		if (result.exponent > fp_info::expMax() || result.exponent < fp_info::expMin()) return false;
		result.whole.end -= result.whole.remainingZeros;
	} else if (result.whole.empty()) {
		if (result.fractional.remainingZeros > fp_info::expMax() - fp_info::expMin()) return false;
		result.exponent -= result.fractional.remainingZeros;

		if (result.exponent > fp_info::expMax() || result.exponent < fp_info::expMin()) return false;
		result.fractional.begin += result.fractional.remainingZeros;
	} else {
		result.exponent -= result.fractional.size();
		if (result.exponent > fp_info::expMax() || result.exponent < fp_info::expMin()) return false;
	}

	return true;
}


// handle trivial cases and hexadecimal floats
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool handleTrivialCases(
	FloatParseResult<Iterator>& parseRes, Numerical& result, CharsFormat fmt
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
	} else if (fmt == CharsFormat::hex) {
		/// @todo

		return true;
	}

	return false;
}


// fast conversion path
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool fastPath(
	const FloatParseResult<Iterator>& parseRes, Numerical& result
) {
	auto wholeLen = parseRes.whole.end - parseRes.whole.begin;
	auto fracLen = parseRes.fractional.end - parseRes.fractional.begin;

	auto e = parseRes.exponent - fracLen - 1;
	auto digitCount = wholeLen + fracLen;

	if (digitCount > 20 || e > parseRes.exponent)
		return false;

	std::uint64_t f = 0;
	if (!uncheckedFastUnsignedFromChars(parseRes.whole.begin, parseRes.whole.end, f, 10)) return false;
	if (!uncheckedFastUnsignedFromChars(parseRes.fractional.begin, parseRes.fractional.end, f, 10)) return false;

	if constexpr (std::is_same_v<Numerical, double>) {
		if (e >= 0) result = f * decDoublePowers[e];
		else result = f / decDoublePowers[-e];
	} else {
		if (e >= 0) result = f * decFloatPowers[e];
		else result = f / decFloatPowers[-e];
	}

	if (parseRes.negative) result *= -1;
	
	return true;
}


// slower path based on the eisel lemire algorithm
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool eiselLemire(
	const FloatParseResult<Iterator>& parseRes, Numerical& result
) {
	using fp_info = FloatingPointInfo<Numerical>;

	typename fp_info::uint_type resInt = 0;

	auto exp = parseRes.exponent + fp_info::expBias();

	if (parseRes.negative) resInt += 1LLU << (sizeof(Numerical) * 8 - 1);
	resInt += implicitCast<std::uint64_t>(detail::powerOfTenExp[exp] + fp_info::expBias()) << fp_info::mantShift();

	/// @todo
	
	result = std::bit_cast<Numerical>(resInt);
	
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
	detail::FloatParseResult<Iterator> parseRes { };

 	if (auto parseErr = detail::parseFloatingPoint(parseRes, begin, end, fmt); parseErr != std::errc { })
		return { begin, parseErr };

	if (detail::handleTrivialCases(parseRes, result, fmt))
		return { parseRes.last, std::errc { } };

	if (!detail::simplifyFloat<Numerical>(parseRes))
		return { begin, std::errc::result_out_of_range };

	if (detail::fastPath(parseRes, result))
		return { parseRes.last, std::errc { } };

	if (detail::eiselLemire(parseRes, result))
		return { parseRes.last, std::errc { } };

	return { begin, std::errc::result_out_of_range }; // since the input should be valid at this point, the only possible error is out of range
}

} // namespace lsd
