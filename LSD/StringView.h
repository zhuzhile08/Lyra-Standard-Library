/************************
 * @file StringView.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief String view implementation
 * 
 * @date 2024-06-12
 * 
 * @copyright Copyright (c) 2024
 ************************/

#pragma once

#include "Hash.h"
#include "Iterators.h"
#include "CharTraits.h"

#include <cstdlib>
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

	using iterator = Iterator<value_type>;
	using const_iterator = Iterator<const_value>; 
	using reverse_iterator = ReverseIterator<value_type>;
	using const_reverse_iterator = ReverseIterator<const_value>; 

	using container = BasicStringView;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	using ostream_type = std::basic_ostream<value_type, std::char_traits<value_type>>;
	using istream_type = std::basic_istream<value_type, std::char_traits<value_type>>;

	static constexpr size_type npos = size_type(-1);

	static_assert(!std::is_array_v<value_type>, "lsd::String: Character type has to be a non-array!");
	static_assert(std::is_trivial_v<value_type>, "lsd::String: Character type has to be trivial!");
	static_assert(std::is_standard_layout_v<value_type>, "lsd::String: Character type has to be in standard layout!");
	static_assert(std::is_same_v<value_type, typename traits_type::char_type>, "lsd::String: Character type has to be the same as the type provided to the character traits!");

	constexpr BasicStringView() noexcept = default;
	constexpr BasicStringView(const_pointer s, size_type count) : m_begin(s), m_end(s + count) { }
	constexpr BasicStringView(const_pointer s) : m_begin(s), m_end(s + traits_type::length(s)) { }
	template <class It, class End> constexpr BasicStringView(It first, End last) 
		requires (isIteratorValue<It> && !std::is_convertible_v<End, size_type> && std::same_as<std::iter_value_t<It>, value_type>) : 
		m_begin(static_cast<const_pointer>(first)),
		m_end(static_cast<const_pointer>(last)) { }
	constexpr BasicStringView(std::nullptr_t) = delete;
	constexpr BasicStringView(const_container_reference) noexcept = default;

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
		auto e = m_end;
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		auto e = m_end;
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		auto b = m_begin;
		return b ? b - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
		auto b = m_begin;
		return b ? b - 1 : nullptr;
	}

	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *m_begin;
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(m_end - 1);
	}
	
	constexpr void removePrefix(size_type n) {
		m_begin += n;
	}
	[[deprecated]] constexpr void remove_prefix(size_type n) {
		removePrefix(n);
	}
	constexpr void removeSuffix(size_type n) {
		m_end -= n;
	}
	[[deprecated]] constexpr void remove_suffix(size_type n) {
		removePrefix(n);
	}

	constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const {
		if (pos > size()) throw std::out_of_range("lsd::BasicStringView::copy(): Position exceded string bounds!");
		return traits_type::copy(dest, m_begin + pos, std::min(count, size() - pos));
	}

	constexpr BasicStringView substr(size_type pos = 0, size_type count = npos) const {
		if (pos > size()) throw std::out_of_range("lsd::BasicStringView::substr(): Position exceded string bounds!");
		return container(m_begin, std::min(count, size() - pos));
	}

	constexpr int compare(container v) const noexcept {
		auto r = traits_type::compare(m_begin, v.m_begin, std::min(size(), v.size()));
		if (r == 0) {
			if (size() < v.size()) return -2;
			else if (size() > v.size()) return 2;
		} else return r;
	}
	constexpr int compare(size_type pos, size_type count, container other) const {
		return substr(pos, count).compare(other);
	}
	constexpr int compare(size_type pos, size_type count, container other, size_type oPos, size_type oCount) const {
		return substr(pos, count).compare(other.substr(oPos, oCount));
	}
	constexpr int compare(const_pointer s) const {
		return compare(container(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s) const {
		return substr(pos, count).compare(container(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s, size_type sCount) const {
		return substr(pos, count).compare(container(s, sCount));
	}

	constexpr bool startsWith(container other) const noexcept {
		return traits_type::compare(m_begin, other.m_begin, std::min(size(), other.size())) == 0;
	}
	[[deprecated]] constexpr bool starts_with(container other) const noexcept {
		return startsWith(other);
	}	
	constexpr bool startsWith(value_type c) const noexcept {
		if (empty()) return false;
		else return traits_type::eq(c, *m_begin);
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
		if (size() < other.size()) return false;
		else return traits_type::compare(m_begin + (size() - other.size()), other.m_begin, other.size()) == 0;
	}
	[[deprecated]] constexpr bool ends_with(container other) const noexcept {
		return endsWith(other);
	}
	constexpr bool endsWith(value_type c) const noexcept {
		if (empty()) return false;
		else return traits_type::eq(c, *(m_end - 1));
	}
	[[deprecated]] constexpr bool ends_with(value_type c) const noexcept {
		return endsWith(c);
	}
	constexpr bool endsWith(const_pointer s) const {
		auto len = traits_type::length(s);
		if (size() < len) return false;
		return traits_type::compare(m_begin + (size() - len), s, len) == 0;
	}
	[[deprecated]] constexpr bool ends_with(const_pointer s) const {
		return endsWith(s);
	}
	
	constexpr bool contains(container other) const noexcept {
		if (size() >= other.size())
			for (auto it = m_begin; it != (m_end - other.size() + 1); it++)
				if (traits_type::compare(other.m_begin, it, other.size()) == 0) return true;

		return false;
	}
	constexpr bool contains(value_type c) const noexcept {
		for (auto it = m_begin; it != m_end; it++) 
			if (traits_type::eq(*it, c)) return true;

		return false;
	}
	constexpr bool contains(const_pointer s) const {
		return contains(container(s));
	}

	constexpr size_type find(const_container_reference other, size_type pos = 0) const noexcept {
		return find(other.data(), pos, other.size());
	}
	constexpr size_type find(const_pointer s, size_type pos, size_type count) const {
		if (size() >= count && pos <= size() - count)
			for (auto it = m_begin + pos; it < (m_end - count); it++)
				if (traits_type::compare(s, it, count) == 0) return it - m_begin;

		return npos;
	}
	constexpr size_type find(const_pointer s, size_type pos = 0) const {
		return find(s, pos, traits_type::length(s));
	}
	constexpr size_type find(value_type c, size_type pos = 0) const noexcept {
		return find(std::addressof(c), pos, 1);
	}

	constexpr size_type rfind(const_container_reference other, size_type pos = npos) const noexcept {
		return rfind(other.data(), pos, other.size());
	}
	constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const {
		pos = std::min(pos, size() - count);

		if (size() >= count)
			for (auto it = (m_begin + pos); it != (m_begin - 1); it--)
				if (traits_type::compare(s, it, count) == 0) return it - m_begin;

		return npos;
	}
	constexpr size_type rfind(const_pointer s, size_type pos = npos) const {
		return rfind(s, pos, traits_type::length(s));
	}
	constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept {
		return rfind(std::addressof(c), pos, 1);
	}

	constexpr size_type findFirstOf(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstOf(other.data(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_first_of(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstOf(other.data(), pos, other.size());
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos, size_type count) const {
		for (auto it = m_begin + pos; it < m_end; it++) 
			for (auto sIt = s; sIt != (s + count); sIt++)
				if (traits_type::eq(*it, *sIt)) return it - m_begin;
			
		return npos;
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos, size_type count) const {
		return findFirstOf(s, pos, count);
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos = 0) const {
		return findFirstOf(s, pos, traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos = 0) const {
		return findFirstOf(s, pos, traits_type::length(s));
	}
	constexpr size_type findFirstOf(value_type c, size_type pos = 0) const noexcept {
		return findFirstOf(std::addressof(c), pos, 1);
	}
	[[deprecated]] constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept {
		return findFirstOf(std::addressof(c), pos, 1);
	}

	constexpr size_type findLastOf(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_last_of(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos, size_type count) const {
		if (size() != 0) {
			pos = std::min(pos, size() - 1);

			for (auto it = m_begin + pos; it > (m_begin - 1); it--) 
				for (auto sIt = s; sIt != (s + count); sIt++)
					if (traits_type::eq(*it, *sIt)) return it - m_begin;
		}

		return npos;
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos, size_type count) const {
		return findLastOf(s, pos, count);
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos = npos) const {
		return findLastOf(s, pos, traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos = npos) const {
		return findLastOf(s, pos, traits_type::length(s));
	}
	constexpr size_type findLastOf(value_type c, size_type pos = npos) const noexcept {
		return findLastOf(std::addressof(c), pos, 1);
	}
	[[deprecated]] constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept {
		return findLastOf(std::addressof(c), pos, 1);
	}

	constexpr size_type findFirstNotOf(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstNotOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstNotOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos, size_type count) const {
		for (auto it = m_begin + pos; it < m_end; it++) {
			bool found = false;
			for (auto sIt = s; sIt != (s + count) && !found; sIt++)
				if (!traits_type::eq(*it, *sIt)) found = true;
			if (!found) return it - m_begin;
		}

		return npos;
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos, size_type count) const {
		return findFirstNotOf(s, pos, count);
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos = 0) const {
		return findFirstNotOf(s, pos, traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos = 0) const {
		return findFirstNotOf(s, pos, traits_type::length(s));
	}
	constexpr size_type findFirstNotOf(value_type c, size_type pos = 0) const noexcept {
		return findFirstNotOf(std::addressof(c), pos, 1);
	}
	[[deprecated]] constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept {
		return findFirstNotOf(std::addressof(c), pos, 1);
	}

	constexpr size_type findLastNotOf(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastNotOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_container_reference other, size_type pos = 0) const noexcept {
		return findLastNotOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos, size_type count) const {
		if (size() != 0) {
			pos = std::min(pos, size() - 1);

			for (auto it = m_end - 1; it > (m_begin + pos - 1); it--) {
				bool found = false;
				for (auto sIt = s; sIt != (s + count) && !found; sIt++)
					if (!traits_type::eq(*it, *sIt)) found = true; 
				if (!found) return it - m_begin;
			}
		}
			
		return npos;
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos, size_type count) const {
		return findLastNotOf(s, pos, count);
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos = npos) const {
		return findLastNotOf(s, pos, traits_type::length(s));
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos = npos) const {
		return findLastNotOf(s, pos, traits_type::length(s));
	}
	constexpr size_type findLastNotOf(value_type c, size_type pos = npos) const noexcept {
		return findLastNotOf(std::addressof(c), pos, 1);
	}
	[[deprecated]] constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept {
		return findLastNotOf(std::addressof(c), pos, 1);
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
		auto ptr = m_begin + index;
		if (ptr >= m_end) throw std::out_of_range("lsd::BasicString::at(): Index exceded string bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const {
		auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::BasicString::operator[]: Index exceded string bounds!");
		return *ptr;
	}

	friend ostream_type& operator<<(ostream_type& stream, const_container_reference string) {
		for (auto it = string.m_begin; it != string.m_end; it++) {
			stream << *it;
		}
		return stream;
	}
	friend istream_type& operator>>(istream_type& stream, const_container_reference string) {
		for (auto it = string.m_begin; it != string.m_end; it++) {
			stream >> *it;
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


template <class C> struct Hash<BasicStringView<C>> {
	using view_type = BasicStringView<C>;

	std::size_t operator()(const view_type& s) const noexcept { // uses the djb2 instead of murmur- or CityHash
		std::size_t hash = 5381; 

#ifdef DJB2_HASH_MULTIPLY_33
#ifdef DJB2_HASH_ADD_CHARACTER
		for (auto it = s.begin(); it != s.end(); it++) hash = hash * 33 + *it;
#else
		for (auto it = s.begin(); it != s.end(); it++) hash = hash * 33 ^ *it;
#endif
#else
#ifdef DJB2_HASH_ADD_CHARACTER
		for (auto it = s.begin(); it != s.end(); it++) hash = ((hash << 5) + hash) + *it;
#else
		for (auto it = s.begin(); it != s.end(); it++) hash = ((hash << 5) + hash) ^ *it;
#endif
#endif

		return hash;
	}
};

} // namespace lsd
