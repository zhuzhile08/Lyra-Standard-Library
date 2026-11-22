/************************
 * @file StringView.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief String view implementation
 * 
 * @date 2024-06-12
 * @copyright Copyright (c) 2024
 ************************/

#pragma once

#include "Hash.h"
#include "Iterators.h"
#include "CharTraits.h"
#include "Detail/String/StringCore.hpp"

#include <cstdlib>
#include <cassert>
#include <utility>
#include <initializer_list>


namespace lsd {

template <class CharTy, class Traits = CharTraits<CharTy>> class BasicStringView {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using traits_type = Traits;

	using value_type = CharTy;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using pointer = value_type*;
	using const_pointer = const_value*;

	using const_iterator = ContinuousIterator<const_value>; 
	using iterator = const_iterator;
	using const_reverse_iterator = ReverseContinuousIterator<const_value>; 
	using reverse_iterator = const_reverse_iterator;

	using container = BasicStringView;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	using ostream_type = std::basic_ostream<value_type, std::char_traits<value_type>>;
	using istream_type = std::basic_istream<value_type, std::char_traits<value_type>>;

	static constexpr size_type npos = string::strNPos;


	static_assert(!std::is_array_v<value_type>, "lsd::StringView: Character type has to be a non-array!");
	static_assert(std::is_trivial_v<value_type>, "lsd::StringView: Character type has to be trivial!");
	static_assert(std::is_standard_layout_v<value_type>, "lsd::StringView: Character type has to be in standard layout!");
	static_assert(std::is_same_v<value_type, typename traits_type::char_type>, "lsd::StringView: Character type has to be the same as the type the character traits provided!");


	constexpr BasicStringView() noexcept = default;
	constexpr BasicStringView(const_container_reference) noexcept = default;
	constexpr BasicStringView(const_pointer s, size_type count) : m_begin(s), m_end(s + count) { }
	constexpr BasicStringView(const_pointer s) : m_begin(s), m_end(s + traits_type::length(s)) { }
	template <std::contiguous_iterator It, std::contiguous_iterator End> constexpr BasicStringView(It first, End last) 
		requires (std::sized_sentinel_for<End, It> && !std::is_convertible_v<End, size_type> && std::same_as<std::iter_value_t<It>, value_type>) :
		m_begin(std::to_address(first)),
		m_end(m_begin + std::distance(first, last)) { }
	constexpr BasicStringView(std::nullptr_t) = delete;


	constexpr container_reference& operator=(const_container_reference) noexcept = default;

	constexpr void swap(container_reference other) noexcept {
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
	}

	
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return m_end;
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return m_end;
	}
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
		return m_end ? m_end - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		return m_end ? m_end - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		return m_begin ? m_begin - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
		return m_begin ? m_begin - 1 : nullptr;
	}

	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *m_begin;
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(m_end - 1);
	}


	constexpr void removePrefix(size_type n) {
		assert((n <= size()) && "lsd::BasiStringView::removePrefix(): Prefix size exceeded string bounds!");
		m_begin += n;
	}
	[[deprecated]] constexpr void remove_prefix(size_type n) {
		removePrefix(n);
	}
	constexpr void removeSuffix(size_type n) {
		assert((n <= size()) && "lsd::BasiStringView::removeSuffix(): Suffix size exceeded string bounds!");
		m_end -= n;
	}
	[[deprecated]] constexpr void remove_suffix(size_type n) {
		removePrefix(n);
	}


	constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const {
		if (pos > size())
			throw std::out_of_range("lsd::BasicStringView::copy(): Position exceeded string bounds!");

		traits_type::copy(dest, m_begin + pos, std::min(count, size() - pos));
		return std::min(count, size() - pos);
	}

	constexpr container substr(size_type pos = 0, size_type count = npos) const {
		if (pos > size())
			throw std::out_of_range("lsd::BasicStringView::substr(): Position exceeded string bounds!");
		return container(m_begin + pos, std::min(count, size() - pos));
	}


	constexpr int compare(container s) const noexcept {
		return string::strcompare<traits_type>(m_begin, size(), s.m_begin, s.size());
	}
	constexpr int compare(size_type pos, size_type count, container other) const {
		if (pos > size())
			throw std::out_of_range("lsd::BasicStringView::compare(): Position exceeded string bounds!");
		return string::strcompare<traits_type>(m_begin + pos, count, other.m_begin, other.size());
	}
	constexpr int compare(size_type pos, size_type count, container other, size_type oPos, size_type oCount) const {
		if (pos > size() || oPos > other.size())
			throw std::out_of_range("lsd::BasicStringView::compare(): Position exceeded string bounds!");
		return string::strcompare<traits_type>(m_begin + pos, count, other.m_begin + pos, oCount);
	}
	constexpr int compare(const_pointer s) const {
		return string::strcompare<traits_type>(m_begin, size(), s, traits_type::length(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s) const {
		if (pos > size())
			throw std::out_of_range("lsd::BasicStringView::compare(): Position exceeded string bounds!");
		return string::strcompare<traits_type>(m_begin + pos, count, s, traits_type::length(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s, size_type sCount) const {
		if (pos > size())
			throw std::out_of_range("lsd::BasicStringView::compare(): Position exceeded string bounds!");
		return string::strcompare<traits_type>(m_begin + pos, count, s, sCount);
	}


	constexpr bool startsWith(container other) const noexcept {
		return traits_type::compare(m_begin, other.m_begin, std::min(size(), other.size())) == 0;
	}
	[[deprecated]] constexpr bool starts_with(container other) const noexcept {
		return startsWith(other);
	}	
	constexpr bool startsWith(value_type c) const noexcept {
		return !empty() && traits_type::eq(c, *m_begin);
	}
	[[deprecated]] constexpr bool starts_with(value_type c) const noexcept {
		return startsWith(c);
	}
	constexpr bool startsWith(const_pointer s) const {
		return traits_type::compare(m_begin, s, std::min(size(), traits_type::length(s))) == 0;
	}
	[[deprecated]] constexpr bool starts_with(const_pointer s) const {
		return startsWith(s);
	}

	constexpr bool endsWith(container other) const noexcept {
		return string::strendsWith<traits_type>(m_begin, m_end, other.m_begin, other.size());
	}
	[[deprecated]] constexpr bool ends_with(container other) const noexcept {
		return endsWith(other);
	}
	constexpr bool endsWith(value_type c) const noexcept {
		return string::strendsWith<traits_type>(m_begin, m_end, c);
	}
	[[deprecated]] constexpr bool ends_with(value_type c) const noexcept {
		return endsWith(c);
	}
	constexpr bool endsWith(const_pointer s) const {
		return string::strendsWith<traits_type>(m_begin, m_end, s, traits_type::length(s));
	}
	[[deprecated]] constexpr bool ends_with(const_pointer s) const {
		return endsWith(s);
	}
	constexpr bool endsWith(const_pointer s, size_type len) const {
		return string::strendsWith<traits_type>(m_begin, m_end, s, len);
	}

	constexpr bool contains(container other) const noexcept {
		return string::strcontains<traits_type>(m_begin, m_end, other.m_begin, other.size());
	}
	constexpr bool contains(value_type c) const noexcept {
		return string::strcontains<traits_type>(m_begin, m_end, c);
	}
	constexpr bool contains(const_pointer s) const {
		return string::strcontains<traits_type>(m_begin, m_end, s, traits_type::length(s));
	}
	constexpr bool contains(const_pointer s, size_type len) const noexcept {
		return string::strcontains<traits_type>(m_begin, m_end, s, len);
	}

	constexpr size_type find(container other, size_type pos = 0) const noexcept {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, other.m_begin, other.size());
	}
	constexpr size_type find(const_pointer s, size_type pos, size_type count) const noexcept {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, s, count);
	}
	constexpr size_type find(const_pointer s, size_type pos = 0) const {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, s, traits_type::length(s));
	}
	constexpr size_type find(value_type c, size_type pos = 0) const noexcept {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, c);
	}

	constexpr size_type rfind(container other, size_type pos = npos) const noexcept {
		return string::strrfind<traits_type>(m_begin, size(), pos, other.m_begin, other.size());
	}
	constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const {
		return string::strrfind<traits_type>(m_begin, size(), pos, s, count);
	}
	constexpr size_type rfind(const_pointer s, size_type pos = npos) const {
		return string::strrfind<traits_type>(m_begin, size(), pos, s, traits_type::length(s));
	}
	constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept {
		return string::strrfind<traits_type>(m_begin, size(), pos, c);
	}

	constexpr size_type findFirstOf(container other, size_type pos = 0) const noexcept {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	[[deprecated]] constexpr size_type find_first_of(container other, size_type pos = 0) const noexcept {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos, size_type count) const {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, s, s + count);
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos, size_type count) const {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, s, s + count);
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos = 0) const {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos = 0) const {
		return string::strfindFirstOf<traits_type, false>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	constexpr size_type findFirstOf(value_type c, size_type pos = 0) const noexcept {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, c);
	}
	[[deprecated]] constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept {
		return string::strfind<traits_type>(m_begin, m_begin + pos, m_end, c);
	}

	constexpr size_type findLastOf(container other, size_type pos = npos) const noexcept {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	[[deprecated]] constexpr size_type find_last_of(container other, size_type pos = npos) const noexcept {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos, size_type count) const {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, s, s + count);
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos, size_type count) const {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, s, s + count);
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos = npos) const {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos = npos) const {
		return string::strfindLastOf<traits_type, false>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	constexpr size_type findLastOf(value_type c, size_type pos = npos) const noexcept {
		return string::strrfind<traits_type>(m_begin, size(), pos, c);
	}
	[[deprecated]] constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept {
		return string::strrfind<traits_type>(m_begin, size(), pos, c);
	}

	constexpr size_type findFirstNotOf(container other, size_type pos = 0) const noexcept {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	[[deprecated]] constexpr size_type find_first_not_of(container other, size_type pos = 0) const noexcept {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos, size_type count) const {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, s, s + count);
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos, size_type count) const {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, s, s + count);
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos = 0) const {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos = 0) const {
		return string::strfindFirstOf<traits_type, true>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	constexpr size_type findFirstNotOf(value_type c, size_type pos = 0) const noexcept {
		return string::strfindFirstNotOf<traits_type>(m_begin, size(), pos, c);
	}
	[[deprecated]] constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept {
		return string::strfindFirstNotOf<traits_type>(m_begin, size(), pos, c);
	}

	constexpr size_type findLastNotOf(container other, size_type pos = npos) const noexcept {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	[[deprecated]] constexpr size_type find_last_not_of(container other, size_type pos = 0) const noexcept {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, other.m_begin, other.m_end);
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos, size_type count) const {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, s, s + count);
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos, size_type count) const {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, s, s + count);
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos = npos) const {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos = npos) const {
		return string::strfindLastOf<traits_type, true>(m_begin, size(), pos, s, s + traits_type::length(s));
	}
	constexpr size_type findLastNotOf(value_type c, size_type pos = npos) const noexcept {
		return string::strfindLastNotOf<traits_type>(m_begin, size(), pos, c);
	}
	[[deprecated]] constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept {
		return string::strfindLastNotOf<traits_type>(m_begin, size(), pos, c);
	}


	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_end - m_begin;
	}
	[[nodiscard]] constexpr size_type length() const noexcept {
		return m_end - m_begin;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return std::numeric_limits<size_type>::max() / sizeof(value_type);
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_begin == m_end;
	}

	[[nodiscard]] constexpr const_pointer data() const noexcept {
		return m_begin;
	}

	[[nodiscard]] constexpr const_reference at(size_type index) const {
		const auto ptr = m_begin + index;
		if (ptr >= m_end)
			throw std::out_of_range("lsd::BasicStringView::at(): Index exceeded string bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
		const auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::BasicStringView::operator[]: Index exceeded string bounds!");
		return *ptr;
	}

	friend constexpr bool operator==(container s1, std::type_identity_t<container> s2) {
		return s1.compare(s2) == 0;
	}
	friend constexpr bool operator==(container s1, const_pointer s2) {
		return s1.compare(s2) == 0;
	}
	friend constexpr auto operator<=>(container s1, std::type_identity_t<container> s2) {
		if constexpr (requires { typename traits_type::comparison_category; })
			return static_cast<traits_type::comparison_category>(s1.compare(s2) <=> 0);
		else return static_cast<std::weak_ordering>(s1.compare(s2) <=> 0);
	}
	friend constexpr auto operator<=>(container s1, const_pointer s2) {
		if constexpr (requires { typename traits_type::comparison_category; })
			return static_cast<traits_type::comparison_category>(s1.compare(s2) <=> 0);
		else return static_cast<std::weak_ordering>(s1.compare(s2) <=> 0);
	}

	friend ostream_type& operator<<(ostream_type& stream, container string) {
		for (auto it = string.m_begin; it != string.m_end; it++) {
			stream << *it;
		}
		return stream;
	}

private:
	const_pointer m_begin { };
	const_pointer m_end { };

	template <class, class, class> friend class BasicString;
};


using StringView = BasicStringView<char>;
using WStringView = BasicStringView<wchar_t>;
using U8StringView = BasicStringView<char8_t>;
using U16StringView = BasicStringView<char16_t>;
using U32StringView = BasicStringView<char32_t>;


inline namespace literals {

inline namespace string_literals {

constexpr StringView operator""_lsv(const char* str, std::size_t len) {
	return StringView { str, len };
}
constexpr U8StringView operator""_lsv(const char8_t* str, std::size_t len) {
	return U8StringView { str, len };
}
constexpr U16StringView operator""_lsv(const char16_t* str, std::size_t len) {
	return U16StringView { str, len };
}
constexpr U32StringView operator""_lsv(const char32_t* str, std::size_t len) {
	return U32StringView { str, len };
}
constexpr WStringView operator""_lsv(const wchar_t* str, std::size_t len) {
	return WStringView { str, len };
}

} // Inline namespace literals

} // Inline namespace string_literals


template <class C> struct Hash<BasicStringView<C>> {
	using char_type = C;
	using view_type = BasicStringView<char_type>;
	using traits_type = view_type::traits_type;

	constexpr std::size_t operator()(view_type s) const noexcept { // Uses the djb2 instead of murmur- or CityHash
		return detail::basicStringHash(s.data(), s.data() + s.size());
	}

	constexpr std::size_t operator()(const C* s) const noexcept {
		return Hash<const C*>()(s);
	}
};


template <class CharTy, class Traits> auto quoted(const lsd::BasicStringView<CharTy, Traits>& str, CharTy delim = CharTy('"'), CharTy escape = CharTy('\\')) {
	return quoted(str.data(), delim, escape);
}


namespace detail {

// Helper concept for string-view-likes
template <class SVType, class Ty> concept StringViewLike = std::is_convertible_v<const Ty&, SVType> && 
														   !std::is_convertible_v<const Ty&, const typename SVType::value_type>;

} // namespace detail

} // namespace lsd
