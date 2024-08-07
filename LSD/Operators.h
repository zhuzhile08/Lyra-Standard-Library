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

#include <type_traits>

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

} // namespace lsd