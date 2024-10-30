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
#include "../../Iterators.h"
#include "../../Array.h"

#include <type_traits>
#include <system_error>
#include <cctype>
#include <bit>

namespace lsd {

namespace detail {

// floating point internal data conversion

template <std::size_t Size, std::enable_if_t<Size == 4 || Size == 8, int> = 0>
auto writeToFloatingPointBits(bool negative, std::uint64_t mant, std::uint64_t exp) {
    if constexpr (Size == 8) {
        std::uint64_t res { };
        std::size_t digitCount = std::bit_width(mant) - 1;

        if (negative) res += (std::uint64_t(1) << 63); // set sign if negative
		res += (exp + 1023) << 52; // set exponent
        res += (mant & ~(1 << digitCount) << (52 - digitCount)); // set mantissa

        return res;
	} else {
        std::uint32_t res { };
        std::size_t digitCount = std::bit_width(mant) - 1;
        
        if (negative) res += (std::uint32_t(1) << 31);  // set sign if negativ
        res += (exp + 127) << 23; // set exponent
        res += (mant << (23 - digitCount)) & ~(1 << 23); // set mantissa

        printf("%u\n", res);

        return res;
    }
}


// digit to required amount of bits

constexpr inline std::size_t hexDigitToRequiredBits(int digit) noexcept {
    constexpr Array<std::size_t, 15> lookup { 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

    if (digit <= '9') return lookup[digit - '1'];
    else if (digit <= 'F') return lookup[digit - 'A' + 10];
    else return lookup[digit - 'a' + 10];
}

constexpr inline std::size_t decDigitToRequiredBits(int digit) noexcept {
    constexpr Array<std::size_t, 9> lookup { 1, 2, 2, 3, 3, 3, 3, 4, 4 };

    return lookup[digit - '1'];
}

} // namespace detail


// iterator range to floating point parser
template <class Numerical, class Iterator, typename std::enable_if_t<
	(std::is_same_v<Numerical, float> ||
	std::is_same_v<Numerical, double>) &&
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>,
int> = 0> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, CharsFormat fmt = CharsFormat::general) {
	using namespace operators;
    using view_type = std::basic_string_view<typename std::iterator_traits<Iterator>::value_type>;

    constexpr static auto maxBinDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<52, 23>();
    constexpr static auto maxHexDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<14, 7>();
    constexpr static auto maxDecDigits = ValueConditional<sizeof(Numerical) == 8, std::size_t>::template get<17, 8>();

    constexpr static auto minExponent = ValueConditional<sizeof(Numerical) == 8, std::int64_t>::template get<-1022, -126>();
    constexpr static auto maxExponent = ValueConditional<sizeof(Numerical) == 8, std::int64_t>::template get<1023, 127>();


    if (begin == end) return { begin, std::errc::invalid_argument };

    auto begIt = begin;
	
	bool negative = true;

	if ((negative = (*begin == '-')) && (++begIt == end)) return { begin, std::errc::invalid_argument };

	switch (*begIt) {
		case 'i': 
		case 'I':
			if ((end - ++begIt) >= 2 && detail::caselessStrNCmp(&*begIt, "NF", 2)) {
                result = negative ? -std::numeric_limits<Numerical>::infinity() : std::numeric_limits<Numerical>::infinity();

                if ((begIt += 2) != end && (end - begIt) >= 5 && detail::caselessStrNCmp(&*begIt, "inity", 5)) return { begIt + 5, std::errc { } };
                else return { begIt, std::errc { } };
            } else return { begin, std::errc::invalid_argument };

			break;

		case 'n':
		case 'N':
			if ((end - ++begIt) >= 2 && detail::caselessStrNCmp(&*begIt, "AN", 2)) {
				result = negative ? -std::numeric_limits<Numerical>::quiet_NaN() : std::numeric_limits<Numerical>::quiet_NaN();
				
				return { begIt + 2, std::errc { } };
			} else return { begin, std::errc::invalid_argument };

			break;
	}

    std::uint64_t mant { };
    std::int64_t exp { };

    view_type whole { };
    view_type frac { };

    std::size_t firstNonZeroDigits { };

    Iterator it = begIt;

    if (fmt == CharsFormat::hex) {
        for (; it != end && *it == '0'; it++) // skips leading zeros
        ;

        begIt = it;

        it = fromChars(it, it + std::min(implicitCast<std::size_t>(end - it), maxHexDigits), mant, 16).ptr; // parse the whole part with a limit
        for (; it != end && detail::isHexDigit(*it); it++) // skip rest of the digits
        ;

        whole = view_type(begIt, it);
        
        if (it != end && *it++ == '.') {
            if (it == end) return { begin, std::errc::invalid_argument };

            std::uint64_t remainingDigits = maxHexDigits - (it - begIt) + 1; // + 1 to compensate for the decimal point skipped earlier

            for (; it != end && *it == '0'; it++, firstNonZeroDigits++) // counts leading zeros
            ;

            begIt = it;

            std::uint64_t fracAsInt { };
            it = fromChars(it, it + std::min(implicitCast<std::uint64_t>(end - it), remainingDigits), fracAsInt, 16).ptr; // parse the fractional part with a limit and check it

            if (fracAsInt == 0) {
                if (mant == 0) { // earlist valid exit
                    result = negative ? -0 : 0;
                    return { it, std::errc { } };
                }
            } else {
                mant <<= (it - begIt) * 4;
                mant += fracAsInt;
            }

            for (; it != end && detail::isHexDigit(*it); it++) // skip rest of the digits
            ;
            frac = view_type(begIt, it);
        } else if (mant == 0) { // repeat of the above
            result = negative ? -0 : 0;
            return { it, std::errc { } };
        }

        if (it != end && *it++ == 'p' && it != end) { // parse the exponent
            if (*it == '+') ++it;
            auto fcPtr = fromChars(it, end, exp, 16).ptr; // no need to check again for end, since fromChars does automatically
            
            if (fcPtr == it || exp > maxExponent || exp < minExponent) return { begin, std::errc::invalid_argument };

            exp *= 4;
            it = fcPtr;
        }

        if (whole.size() == 0) { // calculate the exponent, no bound checks for any of the operations, since the above parts should have ruled them out
            exp -= firstNonZeroDigits * 4 + 4 - detail::hexDigitToRequiredBits(frac[firstNonZeroDigits]);
            begin = it;
        } else {
            exp += whole.size() * 4 - 4 + detail::hexDigitToRequiredBits(whole.front()) - 1;
            begin = it;
        }

        result = std::bit_cast<Numerical>(detail::writeToFloatingPointBits<sizeof(Numerical)>(negative, mant, exp));
    } else {
        

        /*
        for (; it != end && *it == '0'; it++) // skips leading zeros
        ;

        begIt = it;

        it = fromChars(it, it + std::min(implicitCast<std::size_t>(end - it), maxDecDigits), mant, 10).ptr; // parse the whole part with a limit
        for (; it != end && detail::isDecDigit(*it); it++) // skip rest of the digits
        ;

        auto bitWidth = std::bit_width(mant);
        exp = implicitCast<std::int64_t>(bitWidth) - 1;
        whole = view_type(begIt, it);

        printf("%lu\n", exp);
        
        if (it != end && *it++ == '.') {
            if (it == end) return { begin, std::errc::invalid_argument };

            std::uint64_t remainingDigits = maxDecDigits - (it - begIt) + 1; // + 1 to compensate for the decimal point skipped earlier
            begIt = it;

            for (; it != end && *it == '0'; it++, firstNonZeroDigits++) // counts leading zeros
            ;

            std::uint64_t fracAsInt { };
            it = fromChars(it, it + std::min(implicitCast<std::size_t>(end - it), remainingDigits), fracAsInt, 10).ptr; // parse the fractional part with a limit and check it

            if (fracAsInt == 0) {
                if (mant == 0) { // earliest valid exit
                    result = negative ? -0 : 0;
                    return { it, std::errc { } };
                }
            } else {
                std::int64_t remaining = maxBinDigits - bitWidth; 

                if (remaining > 0) {
                    auto powerOfTen = std::pow(10, it - begIt - 1);

                    for (; remaining > 0 && fracAsInt != 0; fracAsInt *= 2, mant <<= 1, remaining--) {
                        if (fracAsInt >= powerOfTen) {
                            mant |= 1;
                            fracAsInt -= powerOfTen;
                        }
                    }
                } else mant >>= -1 * remaining;
            }

            for (; it != end && detail::isDecDigit(*it); it++) // skip rest of the digits
            ;
            frac = view_type(begIt, it);
        } else if (mant == 0) { // also earliest valid exit
            result = negative ? -0 : 0;
            return { it, std::errc { } };
        }

        result = std::bit_cast<Numerical>(detail::writeToFloatingPointBits<sizeof(Numerical)>(negative, mant, exp));

        if ((fmt & CharsFormat::scientific) != 0) {
            if (it != end && *it++ == 'e' && it != end) {
                if (*it == '+') ++it;
                auto fcPtr = fromChars(it, end, exp, 10).ptr; // no need to check again for end, since fromChars does automatically

                if (fcPtr == it || exp > maxExponent || exp < minExponent) return { begin, std::errc::invalid_argument };

                it = fcPtr;
            } else if ((fmt & CharsFormat::fixed) == 0) return { begin, std::errc::invalid_argument };
        }
        */
    }

    return { it, std::errc { } };
}

} // namespace lsd
