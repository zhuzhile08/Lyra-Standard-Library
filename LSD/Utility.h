/*************************
 * @file Utility.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief miscelanious functions
 * 
 * @date 2022-02-03
 * 
 * @copyright Copyright (c) 2022
 *************************/

#pragma once

#include "Iterators.h"

#include <limits>
#include <functional>

#include <cstring>

#include <system_error>

namespace lsd {

inline namespace operators {

inline namespace enum_operators {

// enum utilities

template <class Ty> concept EnumType = std::is_enum_v<Ty>;


// credits to https://gist.github.com/StrikerX3/46b9058d6c61387b3f361ef9d7e00cd4 for these operators!

template <EnumType Enum> constexpr inline Enum operator|(Enum first, Enum second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) |
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}

template <EnumType Enum> Enum constexpr inline operator&(Enum first, Enum second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) &
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}

template <EnumType Enum> Enum constexpr inline operator^(Enum first, Enum second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) ^
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}

template <EnumType Enum> Enum constexpr inline operator~(Enum first) noexcept {
	return static_cast<Enum>(
		~static_cast<std::underlying_type_t<Enum>>(first)
	);
}

template <EnumType Enum> Enum constexpr inline operator|=(Enum& first, Enum second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) |
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}

template <EnumType Enum> Enum constexpr inline operator&=(Enum& first, Enum second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) &
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}

template <EnumType Enum> Enum constexpr inline operator^=(Enum& first, Enum second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) ^
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}

} // inline namespace enum_operators

} // inline namespace operators


// address conversion
template <class Ty> [[nodiscard]] constexpr inline const void* getAddress(const Ty& type) noexcept {
	return static_cast<const void*>(type);
}


// implicit cast

template <class To, class From, std::enable_if_t<std::is_convertible_v<From, To>, int> = 0> [[nodiscard]] constexpr inline To implicitCast(const From& arg) {
	return arg;
}
template <class To, class From, std::enable_if_t<std::is_convertible_v<From, To>, int> = 0> [[deprecated]] [[nodiscard]] constexpr inline To implicit_cast(const From& arg) {
	return arg;
}


// character sequence to numerics conversion

namespace detail {

// utility concepts

template <class Ty> concept IteratableContainer = requires(Ty c1, Ty c2) {
	typename Ty::size_type;
	typename Ty::value_type;
	c1.swap(c2);
};


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
		
	} else {
	
	}

	if (iterationCount != 0) result = res * sign;
	else return { beginCopy, std::errc::invalid_argument };

	return { begin, std::errc { } };
}


// compile time type id generator

using type_id = const void*;

template <class> struct TypeId {
private:
	constexpr static char m_id { };

	template <class> friend constexpr type_id typeId() noexcept;
};

template <class Ty> constexpr type_id typeId() noexcept {
	return &TypeId<Ty>::m_id;
}

} // namespace lsd


namespace std {

// standard library function overloads for custom containers

template <lsd::detail::IteratableContainer Ty> void swap(Ty& a, Ty& b) {
	a.swap(b);
}

template <lsd::detail::IteratableContainer Ty, class Value = typename Ty::value_type> Ty::size_type erase(Ty& container, const Value& value) {
	auto it = std::remove(container.begin(), container.end(), value);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}
template <lsd::detail::IteratableContainer Ty, class Pred> Ty::size_type erase(Ty& container, Pred pred) {
	auto it = std::remove(container.begin(), container.end(), pred);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}

} // namespace std
