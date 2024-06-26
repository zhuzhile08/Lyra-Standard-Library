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


// container concepts

template <class Ty> concept IteratableContainer = requires(Ty c1, Ty c2) {
	typename Ty::size_type;
	typename Ty::value_type;
	c1.swap(c2);
};

} // namespace lsd


namespace std {

// standard library function overloads for custom containers

template <lsd::IteratableContainer Ty> void swap(Ty& a, Ty& b) {
	a.swap(b);
}

template <lsd::IteratableContainer Ty, class Value = typename Ty::value_type> Ty::size_type erase(Ty& container, const Value& value) {
	auto it = std::remove(container.begin(), container.end(), value);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}
template <lsd::IteratableContainer Ty, class Pred> Ty::size_type erase(Ty& container, Pred pred) {
	auto it = std::remove(container.begin(), container.end(), pred);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}

template <class CharTy, class Traits, class Alloc> auto quoted(const lsd::BasicString<CharTy, Traits, Alloc>& str, CharTy delim = CharTy('"'), CharTy escape = CharTy('\\')) {
	return quoted(str.data(), delim, escape);
}
template <class CharTy, class Traits> auto quoted(const lsd::BasicStringView<CharTy, Traits>& str, CharTy delim = CharTy('"'), CharTy escape = CharTy('\\')) {
	return quoted(str.data(), delim, escape);
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
