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

#include "String.h"
#include "StringView.h"

#include <functional>

namespace lsd {

template <typename Ty> [[nodiscard]] constexpr inline const void* getAddress(const Ty& type) noexcept {
	return static_cast<const void*>(type);
}


// string operation

template <template <class...> class Container> [[nodiscard]] inline constexpr Container<String> parse(StringView s, StringView d) noexcept {
	Container<String> r;

	std::size_t begin = 0;
	std::size_t current;

	while ((current = s.find(d, begin)) != String::npos) {
		r.emplace_back(s.substr(begin, current - begin));
		begin = current + d.size();
	}
	
	r.emplace_back(s.substr(begin));

	return r;
}

template <template <class...> class Container> [[nodiscard]] inline constexpr Container<WString> parse(WStringView s, WStringView d) noexcept {
	Container<WString> r;

	std::size_t begin = 0;
	std::size_t current;

	while ((current = s.find(d, begin)) != String::npos) {
		r.emplace_back(s.substr(begin, current - begin));
		begin = current + d.size();
	}
	
	r.emplace_back(s.substr(begin));

	return r;
}


// prime number utility

template <class Integer> inline constexpr bool isPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n == 2 || n == 3)
		return true;
	else if (n <= 1 || n % 2 == 0 || n % 3 == 0)
		return false;
	else for (Integer i = 5; i * i <= n; i += 6)
		if (n % i == 0 || n % (i + 2) == 0)
			return false;
	return true;
};

template <class Integer> inline constexpr Integer nextPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n % 2 == 0)
		--n;

	while (true) {
		n += 2;
		if (isPrime(n)) {
			return n;
		}
	}
};

template <class Integer> inline constexpr Integer lastPrime(Integer n) noexcept requires std::is_integral_v<Integer> {
	if (n % 2 == 0)
		++n;

	while (true) {
		n -= 2;
		if (isPrime(n)) {
			return n;
		}
	}
};


// hash map utility

inline constexpr std::size_t hashmapBucketSizeCheck(std::size_t requested, std::size_t required) noexcept {
	return (requested < required) ? nextPrime(required) : nextPrime(requested);
}

template <class Integer> inline constexpr Integer sizeToIndex(Integer size) noexcept requires std::is_integral_v<Integer> {
	return (size == 0) ? 0 : size - 1;
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


// std::swap general overload

template <class Ty> concept Swappable = requires(Ty a, Ty b) { a.swap(b); };

} // namespace lsd


namespace std {

template <lsd::Swappable Ty> void swap(Ty& a, Ty& b) {
	a.swap(b);
}

} // namespace std


#ifdef LSD_ENUM_UTILITIES

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

namespace lsd {

template <EnumType Enum> struct Hash<Enum> {
	constexpr std::size_t operator()(Enum e) const noexcept {
		return static_cast<std::size_t>(e);
	}
};

template <EnumType Enum> inline String toString(Enum e) noexcept {
	return lsd::toString(static_cast<std::underlying_type_t<Enum>>(e));
}
template <EnumType Enum> inline WString toWString(Enum e) noexcept {
	return lsd::toString(static_cast<std::underlying_type_t<Enum>>(e));
}

} // namespace std

#endif
