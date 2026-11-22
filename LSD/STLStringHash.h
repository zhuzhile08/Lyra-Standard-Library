/************************
 * @file STLStringHash.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Standard library compatibility for string hashing
 * 
 * @date 2026-11-22
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "StringView.h"
#include "String.h"


namespace std {

template <class Char> class hash<lsd::BasicString<Char>> {
public:
	using string_type = lsd::BasicString<Char>;
	using view_type = lsd::BasicStringView<Char>;

	constexpr std::size_t operator()(const string_type& s) const noexcept { // Uses the djb2 instead of murmur- or CityHash
		return lsd::Hash<view_type>()(view_type(s));
	}
};


template <class Char> class hash<lsd::BasicStringView<Char>> {
public:
	using char_type = Char;
	using view_type = lsd::BasicStringView<char_type>;
	using traits_type = view_type::traits_type;

	constexpr std::size_t operator()(view_type s) const noexcept { // Uses the djb2 instead of murmur- or CityHash
		return lsd::detail::basicStringHash(s.data(), s.data() + s.size());
	}
};

} // namespace std

