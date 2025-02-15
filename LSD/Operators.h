/*************************
 * @file Operators.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Additional non-standard operator overloads
 * 
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include <concepts>
#include <type_traits>

namespace lsd {

inline namespace operators {

inline namespace enum_operators {

// Enum utilities

template <class Ty> concept EnumType = std::is_enum_v<Ty>;


// Credits to https://gist.github.com/StrikerX3/46b9058d6c61387b3f361ef9d7e00cd4 for these operators!

template <EnumType Enum> constexpr inline Enum operator|(Enum first, Enum second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) |
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}
template <EnumType Enum, std::integral Num> constexpr inline Enum operator|(Enum first, Num second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) |
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}
template <EnumType Enum, std::integral Num> constexpr inline Enum operator|(Num first, Enum second) noexcept {
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
template <EnumType Enum, std::integral Num> Enum constexpr inline operator&(Enum first, Num second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) &
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}
template <EnumType Enum, std::integral Num> Enum constexpr inline operator&(Num first, Enum second) noexcept {
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
template <EnumType Enum, std::integral Num> Enum constexpr inline operator^(Num first, Enum second) noexcept {
	return static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) ^
		static_cast<std::underlying_type_t<Enum>>(second)
	);
}
template <EnumType Enum, std::integral Num> Enum constexpr inline operator^(Enum first, Num second) noexcept {
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
template <EnumType Enum, std::integral Num> Enum constexpr inline operator|=(Enum& first, Num second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) |
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}
template <EnumType Enum, std::integral Num> Enum constexpr inline operator|=(Num& first, Enum second) noexcept {
	return (first = static_cast<Num>(
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
template <EnumType Enum, std::integral Num> Enum constexpr inline operator&=(Enum& first, Num second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) &
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}
template <EnumType Enum, std::integral Num> Enum constexpr inline operator&=(Num& first, Enum second) noexcept {
	return (first = static_cast<Num>(
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
template <EnumType Enum, std::integral Num> Enum constexpr inline operator^=(Enum& first, Num second) noexcept {
	return (first = static_cast<Enum>(
		static_cast<std::underlying_type_t<Enum>>(first) ^
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}
template <EnumType Enum, std::integral Num> Enum constexpr inline operator^=(Num& first, Enum second) noexcept {
	return (first = static_cast<Num>(
		static_cast<std::underlying_type_t<Enum>>(first) ^
		static_cast<std::underlying_type_t<Enum>>(second)
	));
}

template <EnumType Enum, std::integral Num> bool constexpr inline operator==(Enum first, Num second) noexcept {
	return first == static_cast<Enum>(second);
}
template <EnumType Enum, std::integral Num> bool constexpr inline operator==(Num first, Enum second) noexcept {
	return static_cast<Enum>(first) == second;
}

template <EnumType Enum, std::integral Num> auto constexpr inline operator<=>(Enum first, Num second) noexcept {
	return first <=> static_cast<Enum>(second);
}
template <EnumType Enum, std::integral Num> auto constexpr inline operator<=>(Num first, Enum second) noexcept {
	return static_cast<Enum>(first) <=> second;
}

} // Inline namespace enum_operators

} // Inline namespace operators

} // namespace lsd