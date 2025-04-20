/*************************
 * @file Floating.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief From-Chars functions for floating point numbers
 * 
 * @date 2024-08-08
 * @copyright Copyright (c) 2024
 *************************/

#include "../CoreUtility.h"
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

// Some numerical details about each floating point type

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


// Long mantissa multiplication
template <class Numerical> 
constexpr std::uint64_t expAndRawMantToMant(std::uint64_t a, std::uint64_t b, typename FloatingPointInfo<Numerical>::uint_type& result) noexcept {
	using fp_info = FloatingPointInfo<Numerical>;

	// This function doesn't use auto since "wrong" deductions can take place due to the bit shifts 
	// And the implementation differences on different platforms of std::uint64_t

	std::uint64_t aHi = a >> 32;
	std::uint64_t aLo = a & UINT64_C(0xFFFFFFFF);
	std::uint64_t bHi = b >> 32;
	std::uint64_t bLo = b & UINT64_C(0xFFFFFFFF);

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

	std::uint64_t inCompLow = p1 + (p2 << 32);
	std::uint64_t low = inCompLow + (p4 << 32);
	std::uint64_t carry = (inCompLow < p1 ? 1 : 0) + (low < inCompLow ? 1 : 0);
	std::uint64_t high = (p2 >> 32) + p3 + (p4 >> 32) + carry;

	auto highDigits = 64 - std::countl_zero(high);
	

	// Shift mantissa into position

	if (highDigits > fp_info::mantShift() + 1) {
		auto mantissa = high >> (highDigits - fp_info::mantShift() - 2);

		result = (mantissa + (mantissa & 1)) >> 1; // Rounding
	} else if (highDigits < fp_info::mantShift() + 1) {
		auto shift = fp_info::mantShift() - highDigits + 1;
		auto mantissa = low >> (64 - shift - 1);

		result = ((mantissa + (mantissa & 1)) >> 1) + (high << shift);
	} else result = high + (low & (UINT64_C(1) << 63));

	// Remove upper bit of mantissa
	result &= ~(UINT64_C(1) << fp_info::mantShift());

	return highDigits;
}


// Fast conversion path
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool fastPath(
	const FloatParseResult<Iterator>& parseRes, Numerical& result
) {
	std::int64_t e = parseRes.exponent - parseRes.fracSize;

	if (e >= 0) {
		if constexpr (std::is_same_v<Numerical, double>) {
			if (!parseRes.fastPathAvailable || e >= INT64_C(16))
				return false;
			
			result = parseRes.mantissa * decDoublePowers[e];
		} else {
			if (!parseRes.fastPathAvailable || e >= INT64_C(8))
				return false;

			result = parseRes.mantissa * decFloatPowers[e];
		}
	} else {
		if constexpr (std::is_same_v<Numerical, double>) {
			if (!parseRes.fastPathAvailable || e <= INT64_C(-16))
				return false;
			
			result = parseRes.mantissa / decDoublePowers[-e];
		} else {
			if (!parseRes.fastPathAvailable || e <= INT64_C(-8))
				return false;

			result = parseRes.mantissa / decFloatPowers[-e];
		}
	}

	if (parseRes.negative) result *= -1;
	
	return true;
}


// Slower path based on the eisel lemire algorithm
template <ContinuousIteratorType Iterator, class Numerical> constexpr bool eiselLemire(
	const FloatParseResult<Iterator>& parseRes, Numerical& result
) {
	using fp_info = FloatingPointInfo<Numerical>;

	// Calculate the index into the table
	std::size_t expIndex = parseRes.exponent - parseRes.fracSize - 1 + std::size(powerOfTenTable) / 2;
	const auto& powTen = powerOfTenTable[expIndex];

	// Start moving the values into a result integer

	typename fp_info::uint_type resInt = 0;

	// Calculate exponent and check if it is in range
	std::int64_t exp = powTen.second // Base exponent
		 + expAndRawMantToMant<Numerical>(parseRes.mantissa, powTen.first, resInt) // Since the number is shifted backwards during processing
		 + 63; // Since the real value of the number is shifted backwards to 1 > n <= 0
	if (exp < fp_info::expMin() || exp > fp_info::expMax()) return false;
	
	resInt += (implicitCast<std::uint64_t>(parseRes.negative) << 63);
	resInt += (exp + fp_info::expBias()) << fp_info::mantShift();
	
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

 	if (auto parseErr = detail::parseFloatingPoint(parseRes, begin, end, fmt, result); parseErr == std::errc::operation_canceled)
		return { parseRes.last, std::errc { } };
	else if (parseErr != std::errc { })
		return { begin, parseErr };

	if (detail::fastPath(parseRes, result))
		return { parseRes.last, std::errc { } };

	if (detail::eiselLemire(parseRes, result))
		return { parseRes.last, std::errc { } };

	return { begin, std::errc::result_out_of_range }; // Since the input should be valid at this point, the only possible error is out of range
}

} // namespace lsd
