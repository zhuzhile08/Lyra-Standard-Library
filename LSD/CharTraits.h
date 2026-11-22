/*************************
 * @file CharTraits.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Character traits
 * 
 * @date 2024-06-10
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include <ios>
#include <cstdlib>
#include <type_traits>
#include <algorithm>


namespace lsd {

namespace detail {

template <class CharType, class IntType, class StreamPos> class BasicStandardCharTrait {
public:
	using char_type = CharType;
	using int_type = IntType;
	using off_type = std::streamoff;
	using pos_type = StreamPos;
	using state_type = std::mbstate_t;

	using size_type = std::size_t;


	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* dst, size_type count, char_type value) {
		if (count == 0)
			return dst;
		
		if constexpr (std::is_constant_evaluated() || sizeof(char_type) > 1) {
			for (auto it = dst; count != 0; count--, it++)
				assign(*it, value);
		} else
			std::memset(dst, value, count * sizeof(char_type));
		
		return dst;
	}


	constexpr static char_type* copy(char_type* dst, const char_type* src, size_type count) {
		if (count == 0)
			return dst;

		if consteval {
			std::copy(src, src + count, dst + count);
			return dst;
		} else {
			return static_cast<char_type*>(std::memcpy(dst, src, count * sizeof(char_type)));
		}
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, size_type count) {
		if (count == 0)
			return dst;

		if consteval {
			if (const auto dstEnd = dst + count; dst > src && dst < dstEnd)
				std::copy_backward(src, src + count, dstEnd);
			else
				std::copy(src, src + count, dst);

			return dst;
		} else {
			return static_cast<char_type*>(std::memmove(dst, src, count * sizeof(char_type)));
		}
	}


	constexpr static bool eq(char_type a, char_type b) noexcept {
		if constexpr (std::is_same_v<char_type, char>)
			return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
		else return a == b;
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		if constexpr (std::is_same_v<char_type, char>)
			return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
		else return a < b;
	}

	constexpr static int compare(const char_type* s1, const char_type* s2, size_type count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!eq(*s1, *s2)) return 1;
		}

		return 0;
	}


	constexpr static const char_type* find(const char_type* ptr, size_type count, const char_type& ch) {
		for (; count > 0; ptr++, count--)
			if (eq(*ptr, ch)) return ptr;
		return nullptr;
	}


	constexpr static size_type length(const char_type* s) {
		size_type size = 0;
		for (; !eq(*s, char_type { }); s++, size++) { }
		return size;
	}

	
	constexpr static char_type toCharType(int_type c) noexcept {
		return static_cast<char_type>(c);
	}
	[[deprecated]] constexpr static char_type to_char_type(int_type c) noexcept {
		return toCharType(c);
	}

	constexpr static int_type toIntType(char_type c) noexcept {
		return static_cast<int_type>(c);
	}
	[[deprecated]] constexpr static int_type to_int_type(char_type c) noexcept {
		return toIntType(c);
	}

	constexpr static bool eqIntType(int_type c1, int_type c2) noexcept {
		return c1 == c2;
	}
	[[deprecated]] constexpr static bool eq_int_type(int_type c1, int_type c2) noexcept {
		return eqIntType(c1, c2);
	}

	constexpr static int_type eof() noexcept {
		return EOF;
	}
	constexpr static int_type notEof(int_type e) noexcept {
		return (e == eof()) ? int_type { } : e;
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

} // namespace detail


template <class Ty> class CharTraits;

template <> class CharTraits<char> : public detail::BasicStandardCharTrait<char, int, std::streampos> { };

template <> class CharTraits<wchar_t> : public detail::BasicStandardCharTrait<wchar_t, std::wint_t, std::wstreampos> { };

template <> class CharTraits<char8_t> : public detail::BasicStandardCharTrait<char8_t, unsigned int, std::u8streampos> { };

template <> class CharTraits<char16_t> : public detail::BasicStandardCharTrait<char16_t, uint_least16_t, std::u16streampos> { };

template <> class CharTraits<char32_t> : public detail::BasicStandardCharTrait<char32_t, std::uint_least32_t, std::u32streampos> { };

} // namespace lsd
