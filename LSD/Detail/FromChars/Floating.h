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
	consteval static auto size() noexcept {
		return 32;
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
	consteval static auto size() noexcept {
		return 64;
	}
};


// double unsigned integer
class UnsignedInt128 {
public:
	constexpr UnsignedInt128(std::uint64_t a, std::uint64_t b) noexcept {
		// this function doesn't use auto since "wrong" deductions can take place due to the bit shifts 
		// and the implementation differences on different platforms of std::uint64_t

		std::uint64_t aHi = a >> 32;
		std::uint64_t aLo = a & UINT64_C(0x00000000FFFFFFFF);
		std::uint64_t bHi = b >> 32;
		std::uint64_t bLo = b & UINT64_C(0x00000000FFFFFFFF);

		/**
		 * Visual representation of the multiplication grid
		 * 
		 *             aHi aLo
		 *           * bHi bLo
		 *           __________
		 *         / p4 / p1 /
		 *       /----/----/
		 *  +  / p3 / p2 /
		 * __/____/____/_______
		 * 	 <     result    >
		 */
		std::uint64_t p1 = aLo * bLo;
		std::uint64_t p2 = aLo * bHi;
		std::uint64_t p3 = aHi * bHi;
		std::uint64_t p4 = aHi * bLo;

		low = p1 + (p2 << 32);
		std::uint64_t l = low + (p4 << 32);
        std::uint64_t carry = (low < p1 ? 1 : 0) + (l < low ? 1 : 0);
		low = l;

		high = (p2 >> 32) + p3 + (p4 >> 32) + carry; 
	}

	std::uint64_t low { };
	std::uint64_t high { };
};


// simplify parsed float
template <class Numerical, ContinuousIteratorType Iterator> constexpr bool simplifyFloat(FloatParseResult<Iterator>& result) {
	using fp_info = FloatingPointInfo<Numerical>;

	if (result.fractional.empty()) {
		result.exponent = result.exponent - 1 + result.whole.size();
		result.whole.end -= result.whole.remainingZeros;
	} else if (result.whole.empty()) {
		result.exponent -= result.fractional.remainingZeros;
		result.fractional.begin += result.fractional.remainingZeros;
	} else
		result.exponent = result.exponent - 1 + result.whole.size();

	if (result.exponent > fp_info::expMax() || result.exponent < fp_info::expMin()) return false;

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

	// calculate some stuff

	std::uint64_t m = 0;
	std::size_t digitsParsed = 0;
	if (uncheckedOverflowBoundBase10FastUnsignedFromChars(parseRes.whole.begin, parseRes.whole.end, m, digitsParsed))
		uncheckedOverflowBoundBase10FastUnsignedFromChars(parseRes.fractional.begin, parseRes.fractional.end, m, digitsParsed);

	std::size_t expIndex = parseRes.exponent - digitsParsed + powerOfTenMant.size() / 2;
	UnsignedInt128 mantTimesPow10 = UnsignedInt128(m, powerOfTenMant[expIndex]);

	std::size_t leadingZeros = std::countl_zero(mantTimesPow10.high);
	std::size_t highDigits = 64 - leadingZeros;
	
	std::int64_t exp = powerOfTenExp[expIndex] // base exponent
		 + highDigits // since the number is shifted backwards during processing
		 + 63; // since the real value of the number is shifted backwards to 1 > n <= 0
	if (exp < fp_info::expMin() || exp > fp_info::expMax()) return false;


	// start moving the values into a result integer

	typename fp_info::uint_type resInt = 0;

	if (highDigits - 1 > fp_info::mantShift()) {
		auto mantissa = mantTimesPow10.high >> (highDigits - fp_info::mantShift() - 2);

		resInt += (mantissa + (mantissa & 1)) >> 1; // rounding
	} else if (highDigits <= fp_info::mantShift()) {
		auto shift = fp_info::mantShift() - highDigits + 1;
		auto mantissa = mantTimesPow10.low >> (64 - shift - 1);

		resInt += ((mantissa + (mantissa & 1)) >> 1) + (mantTimesPow10.high << shift);
	} else resInt += mantTimesPow10.high + (mantTimesPow10.low & 1);

	// remove upper bit of mantissa
	resInt ^= UINT64_C(1) << fp_info::mantShift();
	
	resInt += (implicitCast<std::uint64_t>(parseRes.negative) << 63);
	auto e = exp + fp_info::expBias() << fp_info::mantShift();
	resInt += exp + fp_info::expBias() << fp_info::mantShift();
	
	
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
