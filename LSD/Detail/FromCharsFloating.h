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
#include "../Iterators.h"
#include "../Array.h"

#include <type_traits>
#include <system_error>
#include <cctype>

namespace lsd {

namespace detail {

// floating point internal data

template <std::size_t> struct FloatingPointData { };
template <> struct FloatingPointData<4> {
public:
	std::uint32_t val;
};
template <> struct FloatingPointData<8> {
public:
	std::uint64_t val;
};


template <std::size_t Size> constexpr void makeFloatingPointNegative(FloatingPointData<Size>& data) noexcept {
    data.val += (decltype(data.val)(1) << Size * 8 - 1);
}

template <std::size_t Size> constexpr bool setFloatingPointExponent(FloatingPointData<Size>& data, std::int64_t exp) noexcept {
	if constexpr (Size == 8) {
		if (exp > 1023 || exp < -1022) return false;
        else data.val += (exp + 1023) << 52;
	} else {
        if (exp > 127 || exp < -126) return false;
        else data.val += (exp + 127) << 23;
    }

    return true;
}

template <std::size_t Size> constexpr void setFloatingPointMantissa(FloatingPointData<Size>& data, std::uint64_t mantissa) noexcept {
    std::size_t digitCount { };

    for (auto m = mantissa; m > 1; m >>= 1, ++digitCount)
    ;
    mantissa &= ~(1 << digitCount);

    if constexpr (Size == 8) data.val += (mantissa << (52 - digitCount));
    else data.val += (mantissa << (23 - digitCount));
}


// digit validity checks

constexpr inline std::size_t isHexDigit(int digit) noexcept {
    return (digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'F') || (digit >= 'a' && digit <= 'f');
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


// floating point parser

template <class Literal> struct FloatingPointParseResult {
public:
	BasicStringView<Literal> whole { };
	BasicStringView<Literal> frac { };
	std::int64_t exp { };
};

template <class Iterator, typename std::enable_if_t<isIteratorValue<Iterator> && std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>, int> = 0> 
constexpr bool parseFloatingPoint(Iterator begin, Iterator end, CharsFormat fmt, FloatingPointParseResult<typename std::iterator_traits<Iterator>::value_type>& res) {
    using view_type = BasicStringView<typename std::iterator_traits<Iterator>::value_type>;

    Iterator it = begin;
    Iterator begIt = begin;

    bool isWholeZero = true;

    if (fmt == CharsFormat::hex) {
        for (; it != end && isHexDigit(*it); it++) { 
            if (isWholeZero && *it != '0') { // checks for leading zeros
				isWholeZero = false;
				begIt = it;
			}
        }

        if (begin != it) {
            res.whole = view_type(begIt, it);
            ++(begIt = it);

            if (it != end && *it == '.') {
                ++it;

                for (; it != end && isHexDigit(*it); it++);
                
                if (it != begIt) res.frac = view_type(begIt, it);
                else return false;
            }
            
            if (it != end && *it++ == 'p' && it != end) {
                if (*it == '+') ++it;
                
                if (fromChars(it, end, res.exp, 16).ptr != it && res.exp <= std::numeric_limits<std::int64_t>::max() / 4) {
                    res.exp *= 4;
                    /*
                    The following calculations are hard to understand, so here is a brief explaination:
            
                    The mantissa needs to be in the binary scientific format
                    This is to calculate the additional amount needed in the exponent to make that possible

                    If the number > 1, i.e. the whole part is equal to zero:

                    We will need to add a number to the exponent

                    If the number < 1, i.e. the whole part is greater than zero:

                    We will need to subtract a number from the exponent
                    
                    */

                    if (isWholeZero) {
                        auto firstNonZero = res.frac.findFirstNotOf('0');
                        std::int64_t expDiff = firstNonZero * 4 + 4 - ((firstNonZero < res.frac.size()) ? hexDigitToRequiredBits(res.frac[firstNonZero]) : 4);

                        if (res.exp >= std::numeric_limits<std::int64_t>::min() + expDiff) {
                            res.exp -= expDiff;
                            return true;
                        } else return false;
                    } else {
                        std::int64_t expDiff = res.whole.size() * 4 - 4 + hexDigitToRequiredBits(res.whole.front()) - 1;

                        if (res.exp <= std::numeric_limits<std::int64_t>::max() - expDiff) {
                            res.exp += expDiff;
                            return true;
                        } else return false;
                    }
                } else return false;
            } else return false;
        } else return false;
    } else {
        for (; it != end && *it >= '0' && *it <= '9'; it++) { 
            isWholeZero = false;
				begIt = it;
        }

        if (begin != it) {
            res.whole = view_type(begIt, it);
            ++(begIt = it);

            if (*it == '.') {
                ++it;

                for (; it != end && *it >= '0' && *it <= '9'; it++);

                if (it == begIt) return false;
                else res.frac = view_type(begIt, it);
            }
            
            if (*it == 'e') {
                if (*++it == '+') ++it;
                
                if (fromChars(it, end, res.exp, 10).ptr != it) {
                    /*
                    if (isWholeZero) {
                        auto exp = res.exp;
                        res.exp += res.whole.size() * 4 - 4 + decDigitToRequiredBits(res.whole.front());

                        if (res.exp <= exp) return false;
                        else return true;
                    } else {
                        auto exp = res.exp;
                        auto firstNonZero = res.frac.find_first_not_of('0');
                        res.exp -= firstNonZero * 4 + decDigitToRequiredBits(res.frac[firstNonZero]);

                        if (res.exp <= exp) return false;
                        else return true;
                    }
                    */
                    return true;
                } else return false;
            } else return false;
        } else return false;
    }
}

} // namespace detail


template <class Numerical, class Iterator, typename std::enable_if_t<
	(std::is_same_v<Numerical, float> ||
	std::is_same_v<Numerical, double>) &&
	isIteratorValue<Iterator> && 
	std::is_integral_v<typename std::iterator_traits<Iterator>::value_type>,
int> = 0> 
constexpr FromCharsResult<Iterator> fromChars(Iterator begin, Iterator end, Numerical& result, CharsFormat fmt = CharsFormat::general) {
	using namespace operators;

	auto beginCopy = begin;
	
	bool negative = true;

	if ((negative = (*begin == '-'))) ++begin;

	switch (*begin) {
		case 'i': 
		case 'I':
			if (begin != end && (end - ++begin) >= 2 && detail::caselessStrNCmp(&*begin, "nf", 2)) {
                result = negative ? -std::numeric_limits<Numerical>::infinity() : std::numeric_limits<Numerical>::infinity();

                begin += 2;
                if ((end - begin) >= 5 && detail::caselessStrNCmp(&*begin, "inity", 5)) return { begin + 5, std::errc { } };
                else return { begin, std::errc { } };
            } else return { beginCopy, std::errc::invalid_argument };

			break;

		case 'n':
		case 'N':
			if (begin != end && (end - ++begin) >= 2 && detail::caselessStrNCmp(&*begin, "an", 2)) {
				result = negative ? -std::numeric_limits<Numerical>::quiet_NaN() : std::numeric_limits<Numerical>::quiet_NaN();
				
				return { begin + 2, std::errc { } };
			} else return { beginCopy, std::errc::invalid_argument };

			break;
	}

    detail::FloatingPointParseResult<typename std::iterator_traits<Iterator>::value_type> parseResult { };

    if (detail::parseFloatingPoint(begin, end, fmt, parseResult)) {
        detail::FloatingPointData<sizeof(Numerical)> floatingPointData { };

        detail::setFloatingPointExponent<sizeof(Numerical)>(floatingPointData, parseResult.exp);

        std::uint64_t mantissa { };

        if (fmt == CharsFormat::hex) {
            if (parseResult.whole.size() < 14) {
                fromChars(parseResult.whole.begin(), parseResult.whole.end(), mantissa, 16);

                std::uint64_t fractionalAsInt { };
                fromChars(parseResult.frac.begin(), parseResult.frac.begin() + std::min(14 - parseResult.whole.size(), parseResult.frac.size()), fractionalAsInt, 16);

                for (auto c = parseResult.frac.size(); c > 0; c--) mantissa *= 16;

                mantissa += fractionalAsInt;
            } else
                fromChars(parseResult.whole.begin(), parseResult.whole.begin() + 14, mantissa, 16);
        } else {

        }

        detail::setFloatingPointMantissa<sizeof(Numerical)>(floatingPointData, mantissa);

        if (negative) detail::makeFloatingPointNegative(floatingPointData);

        result = std::bit_cast<Numerical>(floatingPointData);

        return { begin, std::errc { } };
    } else return { beginCopy, std::errc::invalid_argument };
}

} // namespace lsd
