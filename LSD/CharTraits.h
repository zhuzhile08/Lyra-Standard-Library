/*************************
 * @file CharTraits.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Character traits
 * 
 * @date 2024-06-10
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include <ios>
#include <cstdlib>
#include <utility>
#include <algorithm>

namespace lsd {

template <class Ty> class CharTraits;

template <> class CharTraits<char> {
public:
	using char_type = char;
	using int_type = int;
	using off_type = std::streamoff;
	using pos_type = std::streampos;
	using state_type = std::mbstate_t;

	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* ptr, std::size_t count, char_type c2) {
		std::fill_n(ptr, count, c2);
		return ptr;
	}

	constexpr static bool eq(char_type a, char_type b) noexcept {
		return a == b;
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		return a < b;
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::move(src, src + count, dst);
		else if (dst > src) return std::move_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static char_type* copy(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::copy(src, src + count, dst);
		else if (dst > src) return std::copy_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static int compare(const char_type* s1, const char_type* s2, std::size_t count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!lt(*s1, *s2) && !eq(*s1, *s2)) return 1;
		}

		return 0;
	}

	constexpr static std::size_t length(const char_type* s) {
		std::size_t size = 0;

		for (; !eq(*s, '\0'); s++, size++) { }

		return size;
	}

	constexpr static const char_type* find(const char_type* ptr, std::size_t count, const char_type& ch) {
		for (; count > 0; ptr++, count--) if (eq(*ptr, ch)) return ptr;
		return nullptr;
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
		return e == eof();
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

template <> class CharTraits<wchar_t> {
public:
	using char_type = wchar_t;
	using int_type = std::wint_t;
	using off_type = std::streamoff;
	using pos_type = std::wstreampos;
	using state_type = std::mbstate_t;

	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* ptr, std::size_t count, char_type c2) {
		std::fill_n(ptr, count, c2);
		return ptr;
	}

	constexpr static bool eq(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::move(src, src + count, dst);
		else if (dst > src) return std::move_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static char_type* copy(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::copy(src, src + count, dst);
		else if (dst > src) return std::copy_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static bool compare(const char_type* s1, const char_type* s2, std::size_t count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!lt(*s1, *s2) && !eq(*s1, *s2)) return 1;
		}

		return 0;
	}

	constexpr static std::size_t length(const char_type* s) {
		std::size_t size = 0;

		for (; !eq(*s, '\0'); s++, size++) { }

		return size;
	}

	constexpr static const char_type* find(const char_type* ptr, std::size_t count, const char_type& ch) {
		for (; count > 0; ptr++, count--) if (eq(*ptr, ch)) return ptr;
		return nullptr;
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
		return e == eof();
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

template <> class CharTraits<char8_t> {
public:
	using char_type = char8_t;
	using int_type = unsigned int;
	using off_type = std::streamoff;
	using pos_type = std::u8streampos;
	using state_type = std::mbstate_t;

	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* ptr, std::size_t count, char_type c2) {
		std::fill_n(ptr, count, c2);
		return ptr;
	}

	constexpr static bool eq(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::move(src, src + count, dst);
		else if (dst > src) return std::move_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static char_type* copy(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::copy(src, src + count, dst);
		else if (dst > src) return std::copy_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static bool compare(const char_type* s1, const char_type* s2, std::size_t count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!lt(*s1, *s2) && !eq(*s1, *s2)) return 1;
		}

		return 0;
	}

	constexpr static std::size_t length(const char_type* s) {
		std::size_t size = 0;

		for (; !eq(*s, '\0'); s++, size++) { }

		return size;
	}

	constexpr static const char_type* find(const char_type* ptr, std::size_t count, const char_type& ch) {
		for (; count > 0; ptr++, count--) if (eq(*ptr, ch)) return ptr;
		return nullptr;
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
		return e == eof();
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

template <> class CharTraits<char16_t> {
public:
	using char_type = char16_t;
	using int_type = std::uint_least16_t;
	using off_type = std::streamoff;
	using pos_type = std::u16streampos;
	using state_type = std::mbstate_t;

	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* ptr, std::size_t count, char_type c2) {
		std::fill_n(ptr, count, c2);
		return ptr;
	}

	constexpr static bool eq(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::move(src, src + count, dst);
		else if (dst > src) return std::move_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static char_type* copy(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::copy(src, src + count, dst);
		else if (dst > src) return std::copy_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static bool compare(const char_type* s1, const char_type* s2, std::size_t count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!lt(*s1, *s2) && !eq(*s1, *s2)) return 1;
		}

		return 0;
	}

	constexpr static std::size_t length(const char_type* s) {
		std::size_t size = 0;

		for (; !eq(*s, '\0'); s++, size++) { }

		return size;
	}

	constexpr static const char_type* find(const char_type* ptr, std::size_t count, const char_type& ch) {
		for (; count > 0; ptr++, count--) if (eq(*ptr, ch)) return ptr;
		return nullptr;
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
		return e == eof();
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

template <> class CharTraits<char32_t> {
public:
	using char_type = char32_t;
	using int_type = std::uint_least32_t;
	using off_type = std::streamoff;
	using pos_type = std::u32streampos;
	using state_type = std::mbstate_t;

	constexpr static void assign(char_type& c1, const char_type& c2) noexcept {
		c1 = c2;
	}
	constexpr static char_type* assign(char_type* ptr, std::size_t count, char_type c2) {
		std::fill_n(ptr, count, c2);
		return ptr;
	}

	constexpr static bool eq(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) == static_cast<unsigned char>(b);
	}
	constexpr static bool lt(char_type a, char_type b) noexcept {
		return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
	}

	constexpr static char_type* move(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::move(src, src + count, dst);
		else if (dst > src) return std::move_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static char_type* copy(char_type* dst, const char_type* src, std::size_t count) {
		if (dst < src) return std::copy(src, src + count, dst);
		else if (dst > src) return std::copy_backward(src, src + count, dst + count);
		else return dst;
	}

	constexpr static bool compare(const char_type* s1, const char_type* s2, std::size_t count) {
		for (; count > 0; count--, s1++, s2++) {
			if (lt(*s1, *s2)) return -1;
			else if (!lt(*s1, *s2) && !eq(*s1, *s2)) return 1;
		}

		return 0;
	}

	constexpr static std::size_t length(const char_type* s) {
		std::size_t size = 0;

		for (; !eq(*s, '\0'); s++, size++) { }

		return size;
	}

	constexpr static const char_type* find(const char_type* ptr, std::size_t count, const char_type& ch) {
		for (; count > 0; ptr++, count--) if (eq(*ptr, ch)) return ptr;
		return nullptr;
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
		return e == eof();
	}
	[[deprecated]] constexpr static int_type not_eof(int_type e) noexcept {
		return notEof(e);
	}
};

} // namespace lsd
