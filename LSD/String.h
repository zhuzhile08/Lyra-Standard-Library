/*************************
 * @file String.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief String implementation
 * 
 * @date 2024-02-24
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Hash.h"
#include "Iterators.h"
#include "CharTraits.h"
#include "StringView.h"

#include <cstdlib>
#include <cassert>
#include <new>	
#include <utility>
#include <initializer_list>
#include <ostream>
#include <istream>

namespace lsd {

// utility macros

#define SIGNED_SCALAR_DIGITS(type) ((sizeof(type) / 2) * 3 + sizeof(type)) + 2
#define UNSIGNED_SCALAR_DIGITS(type) ((sizeof(type) / 2) * 3 + sizeof(type)) + 1


template <class CharTy, class Traits = CharTraits<CharTy>, class Alloc = std::allocator<CharTy>> class BasicString { // @todo custom compile time allocator implementation
public: 
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using traits_type = Traits;
	using allocator_type = Alloc;
	using const_alloc_reference = const allocator_type&;
	using allocator_traits = std::allocator_traits<allocator_type>;

	using value_type = CharTy;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using pointer = value_type*;
	using const_pointer = const_value*;

	using string_tag_type = unsigned char;

	using iterator = Iterator<value_type>;
	using const_iterator = Iterator<const_value>; 
	using reverse_iterator = ReverseIterator<value_type>;
	using const_reverse_iterator = ReverseIterator<const_value>; 

	using container = BasicString;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	using view_type = BasicStringView<value_type, traits_type>;
	using std_view_type = std::basic_string_view<value_type, traits_type>;
	using ostream_type = std::basic_ostream<value_type, std::char_traits<value_type>>;
	using istream_type = std::basic_istream<value_type, std::char_traits<value_type>>;

	static constexpr size_type npos = -1;

private:
	// small string and padding calculation

	static constexpr size_type stringDataSize = sizeof(pointer) * 3;
	static constexpr size_type smallStringCap = size_type(stringDataSize / sizeof(value_type) + 1);
	static constexpr size_type smallStringMax = smallStringCap - 1;
	static constexpr size_type paddingSize = smallStringCap * sizeof(value_type) - stringDataSize;

	using small_string_type = value_type[smallStringCap];


	// internal string storage types

	struct Short {
		small_string_type data { };
		string_tag_type tag { 1 }; // 1 means small string state, 0 means regular string state
	};

	struct Long {
		pointer begin { };
		pointer end { };
		pointer cap { };

		unsigned char padding[paddingSize] { }; // unused
	};


	// eraseAndInsertGap special return value
	struct EraseAndInsertGapReturnInfo {
		pointer result;
		bool isMemReady;
	};


	// helper type trait for string-view-likes

	template <class StringViewLike, class = void> struct IsConvertibleToView : public std::false_type { };
	template <class StringViewLike> struct IsConvertibleToView<StringViewLike, std::enable_if_t<
		std::is_convertible_v<const StringViewLike&, view_type> && 
		!std::is_convertible_v<const StringViewLike&, const_pointer>
	>> : public std::true_type { };

	template <class StringViewLike> static constexpr bool isConvertibleToView = IsConvertibleToView<StringViewLike>::value;

public:
	static_assert(!std::is_array_v<value_type>, "lsd::String: Character type has to a non-array!");
	static_assert(std::is_trivial_v<value_type>, "lsd::String: Character type has to trivial!");
	static_assert(std::is_standard_layout_v<value_type>, "lsd::String: Character type has to  in standard layout!");
	static_assert(std::is_same_v<value_type, typename traits_type::char_type>, "lsd::String: Character type has to be the same as the type provided to the character traits!");

	constexpr BasicString() noexcept(noexcept(allocator_type())) : BasicString(allocator_type()) { }
	constexpr explicit BasicString(const_alloc_reference alloc) : m_alloc(alloc) { }
	constexpr BasicString(size_type count, value_type value, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc) {
		resize(count, value);
	}
	constexpr BasicString(const_container_reference other, size_type pos, const_alloc_reference alloc = allocator_type()) {
		if (pos > other.size()) throw std::out_of_range("lsd::BasicString::BasicString(): Position exceded string bounds!");
		else *this = std::move(BasicString(other.pBegin() + pos, other.pEnd(), alloc));
	}
	constexpr BasicString(container_rvreference other, size_type pos, const_alloc_reference alloc = allocator_type()) : m_alloc(alloc) {
		if (pos > other.size()) throw std::out_of_range("lsd::BasicString::BasicString(): Position exceded string bounds!");
		else {
			if constexpr (allocator_traits::is_always_equal::value || !allocator_traits::propagate_on_container_move_assignment::value) 
				*this = std::move(other);
			else {
				if (m_alloc == other.m_alloc) *this = std::move(other);
				else *this = std::move(BasicString(other.pBegin() + pos, other.pEnd(), alloc));
			}
		}
		
	}
	constexpr BasicString(const_container_reference other, size_type pos, size_type count, const_alloc_reference alloc = allocator_type()) {
		if (pos > other.size()) throw std::out_of_range("lsd::BasicString::BasicString(): Position exceded string bounds!");
		else *this = std::move(BasicString(other.pBegin() + pos, other.pBegin() + pos + std::min(count, other.size() - pos), alloc));
	}
	constexpr BasicString(container_rvreference other, size_type pos, size_type count, const_alloc_reference alloc = allocator_type()) {
		if (pos > other.size()) throw std::out_of_range("lsd::BasicString::BasicString(): Position exceded string bounds!");
		else *this = std::move(BasicString(other.pBegin() + pos, other.pBegin() + pos + std::min(count, other.size() - pos), alloc));
	}
	constexpr BasicString(const_pointer s, size_type count, const_alloc_reference alloc = allocator_type())
		 : BasicString(s, s + count, alloc) { }
	constexpr BasicString(const_pointer s, const_alloc_reference alloc = allocator_type())
		 : BasicString(s, s + traits_type::length(s), alloc) { }
	template <class It> constexpr BasicString(It first, It last, const_alloc_reference alloc = allocator_type()) requires isIteratorValue<It> : 
		m_alloc(alloc) {
		if (first != last) {
			auto count = last - first;
			reserve(count);

			if (smallStringMode())
				for (auto it = m_short.data; first != last; it++, first++) traits_type::assign(*it, *first);
			else {
				for (; first != last; m_long.end++, first++) allocator_traits::construct(m_alloc, m_long.end, *first);
				allocator_traits::construct(m_alloc, m_long.end, value_type { });
			}
		}
	}
	constexpr BasicString(const_container_reference other) : BasicString(other.pBegin(), other.pEnd(), other.m_alloc) { }
	constexpr BasicString(const_container_reference other, const_alloc_reference alloc) : BasicString(other.pBegin(), other.pEnd(), alloc) { }
	constexpr BasicString(container_rvreference other) noexcept :
		m_alloc(std::exchange(other.m_alloc, m_alloc)) {
		if (other.smallStringMode()) {
			m_short.tag = 1;
			traits_type::move(m_short.data, other.m_short.data, smallStringCap);
		} else {
			m_short.tag = std::exchange(other.m_short.tag, 1); // other is now practically in small string mode
			m_long.begin = std::exchange(other.m_long.begin, pointer { });
			m_long.end = std::exchange(other.m_long.end, pointer { });
			m_long.cap = std::exchange(other.m_long.cap, pointer { });
		}
	}
	constexpr BasicString(container_rvreference other, const_alloc_reference alloc) : 
		m_alloc(alloc) {
		if (other.smallStringMode()) {
			m_short.tag = 1;
			traits_type::move(m_short.data, other.m_short.data, smallStringCap);
		} else {
			m_long.begin = std::exchange(other.m_long.begin, pointer { });
			m_long.end = std::exchange(other.m_long.end, pointer { });
			m_long.cap = std::exchange(other.m_long.cap, pointer { });
		}
	}
	constexpr BasicString(init_list ilist, const_alloc_reference alloc = allocator_type())
		 : BasicString(ilist.begin(), ilist.end(), alloc) { }
	template <class StringViewLike> constexpr BasicString(const StringViewLike& sv, const_alloc_reference alloc = allocator_type()) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);
		*this = std::move(BasicString(v.m_begin, v.m_end, alloc));
	}
	template <class StringViewLike> constexpr BasicString(const StringViewLike& sv, size_type pos, size_type count, const_alloc_reference alloc = allocator_type()) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);
		if (pos > v.size()) throw std::out_of_range("lsd::BasicString::BasicString(): Position exceded string view bounds!");
		else *this = std::move(BasicString(v.m_begin + pos, v.m_begin + pos + std::min(count, v.size() - pos), alloc));
	}
	
	constexpr ~BasicString() {
		if (!smallStringMode()) {
			destructBehind(m_long.begin);

			allocator_traits::deallocate(m_alloc, m_long.begin, m_long.cap - m_long.begin);
			
			m_long.begin = nullptr;
			m_long.end = nullptr;
			m_long.cap = nullptr;
		}
	}

	constexpr container_reference operator=(const_container_reference other) {
		return assign(other.pBegin(), other.pEnd());
	}
	constexpr container_reference operator=(container_rvreference other) noexcept {
		std::swap(other.m_alloc, m_alloc);
		std::swap(other.m_short.tag, m_short.tag);

		if (smallStringMode()) {
			traits_type::move(m_short.data, other.m_short.data, smallStringCap);
		} else {
			std::swap(other.m_long.begin, m_long.begin);
			std::swap(other.m_long.end, m_long.end);
			std::swap(other.m_long.cap, m_long.cap);
		}

		return *this;
	}
	constexpr container_reference operator=(const_pointer s) {
		return assign(s, s + traits_type::length(s));
	}
	constexpr container_reference operator=(value_type c) {
		return assign(1, c);
	}
	constexpr container_reference operator=(init_list ilist) {
		return assign(ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference operator=(const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);
		return assign(v.m_begin, v.m_end);
	}
	constexpr container_reference operator=(std::nullptr_t) = delete;

	constexpr container_reference assign(size_type count, value_type value) {
		clear();
		resize(count, value);
		return *this;
	}
	constexpr container_reference assign(const_container_reference other) {
		m_alloc = other.m_alloc;
		assign(other.pBegin(), other.pEnd());
		return *this;
	}
	constexpr container_reference assign(const_container_reference other, size_type pos, size_type count = npos) {
		auto s = other.size(); // just in case to avoid traits_type::length()
		if (pos > s) throw std::out_of_range("lsd::BasicString::operator=(): Requested position exceded string bounds!");

		m_alloc = other.m_alloc;
		assign(other.pBegin() + pos, other.pBegin() + pos + std::min(count, s - pos));
		return *this;
	}
	constexpr container_reference assign(container_rvreference other) noexcept {
		*this = std::move(other);
		return *this;
	}
	constexpr container_reference assign(const_pointer s, size_type count) {
		return assign(s, s + count);
	}
	constexpr container_reference assign(const_pointer s) {
		return assign(s, s + traits_type::length(s));
	}
	template <class It> constexpr container_reference assign(It first, It last) requires isIteratorValue<It> {
		clear();

		if (first != last) {
			auto count = last - first;
			smartReserve(count);

			if (smallStringMode()) 
				for (auto it = m_short.data; first != last; it++, first++) traits_type::assign(*it, *first);
			else {
				for (; first != last; first++, m_long.end++) allocator_traits::construct(m_alloc, m_long.end, *first);
				allocator_traits::construct(m_alloc, m_long.end, '\0');
			}
		}

		return *this;
	}
	constexpr container_reference assign(init_list ilist) {
		return assign(ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference assign(const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);
		return assign(v.m_begin, v.m_end);
	}
	template <class StringViewLike> constexpr container_reference assign(const StringViewLike& sv, size_type pos, size_type count = npos) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);
		if (pos > v.size()) throw std::out_of_range("lsd::BasicString::operator=(): Requested position exceded string bounds!");

		return assign(v.m_begin + pos, v.m_begin + pos + std::min(count, v.size() - pos));
	}

	constexpr void swap(container_reference other) {
		std::swap(other.m_short.tag, m_short.tag);

		auto so = std::move(other);
		auto st = std::move(*this);

		*this = std::move(so);
		other = std::move(st);
	}

	[[nodiscard]] constexpr iterator begin() noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return pEnd();
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return pEnd();
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return pEnd();
	}
	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
		auto e = pEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
		auto e = pEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		auto e = pEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr reverse_iterator rend() noexcept {
		auto b = pBegin();
		return b ? b - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		auto b = pBegin();
		return b ? b - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
		auto b = pBegin();
		return b ? b - 1 : nullptr;
	}

	[[nodiscard]] constexpr reference front() noexcept {
		return *pBegin();
	}
	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *pBegin();
	}
	[[nodiscard]] constexpr reference back() noexcept {
		return *(pEnd() - 1);
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(pEnd() - 1);
	}
	
	constexpr void resize(size_type count) {
		auto s = size();
		if (count > s)
			append(count - s, value_type { });
		else if (count < s)
			destructBehind(pBegin() + count);
	}
	constexpr void resize(size_type count, const_reference value) {
		auto s = size();
		if (count > s)
			append(count - s, value);
		else if (count < s) 
			destructBehind(pBegin() + count);
	}
	template <class Operation> constexpr void resize_and_overwrite(size_type count, Operation op) {
		resizeAndOverwrite(count, op);
	}
	constexpr void reserve(size_type count) {
		++count; // null terminator

		if (count > maxSize()) throw std::length_error("lsd::BasicString::reserve(): Count + 1 exceded maximum allocation size");
		else {
			if (smallStringMode() && count > smallStringCap) {
				auto ssSize = smallStringSize(); 

				pointer begin { };
				begin = allocator_traits::allocate(m_alloc, count);

				auto beginIt = begin;
				for (auto ssIt = m_short.data; ssIt <= (m_short.data + ssSize); ssIt++, beginIt++)  // <= because of null terminator
					allocator_traits::construct(m_alloc, beginIt, *ssIt);

				m_long.begin = begin;
				m_long.end = m_long.begin + ssSize;
				m_long.cap = m_long.begin + count;
				m_short.tag = 0;
			} else if (!smallStringMode()) {
				auto cap = capacity();

				if (count > cap) {
					auto s = size();
					auto oldBegin = std::exchange(m_long.begin, allocator_traits::allocate(m_alloc, count));

					if (oldBegin) {
						for (auto beginIt = m_long.begin, oldBeginIt = oldBegin; oldBeginIt != m_long.end + 1; oldBeginIt++, beginIt++) // plus one for null terminator
							allocator_traits::construct(m_alloc, beginIt++, *oldBeginIt++);

						allocator_traits::deallocate(m_alloc, oldBegin, cap);
					}

					m_long.cap = m_long.begin + count;
					m_long.end = m_long.begin + s;
				}
			}
		}
	}
	constexpr void shrinkToFit() {
		if (!smallStringMode()) {
			auto s = size() + 1; // + 1 because of the null terminator
			auto cap = capacity();

			if (s < cap) {
				if (s <= smallStringCap) {
					pointer oldBegin = m_long.begin;
					pointer oldEnd = m_long.end + 1;
					m_short.tag = 1;

					m_short.data = { };
					for (size_type i = 0; oldBegin != oldEnd; oldBegin++, i++) traits_type::assign(m_short.data[i], *oldBegin);

					allocator_traits::deallocate(m_alloc, oldBegin, cap);
				} else {
					auto oldBegin = std::exchange(m_long.begin, allocator_traits::allocate(m_alloc, s));

					if (oldBegin) {
						for (auto beginIt = m_long.begin, oldBeginIt = oldBegin; oldBeginIt != m_long.end + 1; beginIt++, oldBeginIt++)
							allocator_traits::construct(m_alloc, beginIt, *oldBeginIt);

						allocator_traits::deallocate(m_alloc, oldBegin, cap);
					}

					m_long.end = m_long.begin + s;
					m_long.cap = m_long.end;
				}
			}
		}
	}
	[[deprecated]] constexpr void shrink_to_fit() {
		shrinkToFit();
	}

	constexpr container_reference insert(size_type index, size_type count, value_type c) {
		insert(m_long.begin + index, count, c);
		
		return *this;
	}
	constexpr container_reference insert(size_type index, const_pointer s) {
		insert(m_long.begin + index, s, s + traits_type::length(s));
		
		return *this;
	}
	constexpr container_reference insert(size_type index, const_pointer s, size_type count) {
		insert(m_long.begin + index, s, s + count);
		
		return *this;
	}
	constexpr container_reference insert(size_type index, const_container_reference str) {
		insert(m_long.begin + index, str.m_long.begin, str.m_long.end);

		return *this;
	}
	constexpr container_reference insert(size_type index, const_container_reference str, size_type sIndex, size_type count = npos) {
		insert(m_long.begin + index, str.m_long.begin + sIndex, str.m_long.begin + sIndex + std::min(count, str.size() - sIndex));
		
		return *this;
	}
	constexpr iterator insert(const_iterator position, value_type value) {
		return insert(position, 1, value);
	}
	constexpr iterator insert(const_iterator position, size_type count, value_type value) {
		auto pos = const_cast<pointer>(position.get());

		if (count != 0) {
			auto info = eraseAndInsertGap(pos, 0, count);

			if (info.isMemReady)
				for (; count != 0; count--, info.result++) traits_type::assign(*info.result, value);
			else
				for (; count != 0; count--, info.result++) allocator_traits::construct(m_alloc, info.result, value);
		} else return pos;
	}
	template <class It> constexpr iterator insert(const_iterator position, It first, It last) requires isIteratorValue<It> {
		auto pos = const_cast<pointer>(position.get());
		
		if (first != last) {
			auto info = eraseAndInsertGap(pos, 0, last - first);

			if (info.isMemReady)
				for (; first != last; first++, info.result++) traits_type::assign(*info.result, *first);
			else
				for (; first != last; first++, info.result++) allocator_traits::construct(m_alloc, info.result, *first);
		} else return pos;
	}
	constexpr iterator insert(const_iterator pos, init_list ilist) {
		return insert(pos, ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference insert(size_type index, const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		insert(m_long.begin + index, sv.m_begin, sv.m_end);
		
		return *this;
	}
	template <class StringViewLike> constexpr container_reference insert(size_type index, const StringViewLike& sv, size_type svIndex, size_type count = npos) requires isConvertibleToView<StringViewLike> {
		insert(m_long.begin + index, sv.m_begin + svIndex, sv.m_begin + svIndex + std::min(count, sv.size() - svIndex));
		
		return *this;
	}

	constexpr void pushBack(value_type value) {
		this->operator+=(value);
	}
	[[deprecated]] constexpr void push_back(value_type value) {
		pushBack(value);
	}

	constexpr container_reference replace(size_type pos, size_type count, const_container_reference str) {
		return replace(pBegin() + pos, pBegin() + pos + count, str.pBegin(), str.pEnd());
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, const_container_reference str) {
		return replace(first, last, str.pBegin(), str.pEnd());
	}
	constexpr container_reference replace(size_type pos, size_type count, const_container_reference str, size_type sPos, size_type sCount = npos) {
		if (sPos > str.size()) throw std::out_of_range("lsd::BasicString::replace(): Position exceded bounds of inserted string!");

		auto beg = pBegin() + pos;
		auto sBeg = str.pBegin() + sPos;

		return replace(beg, beg + count, sBeg, sBeg + std::min(sCount, str.size() - sPos));
	}
	constexpr container_reference replace(size_type pos, size_type count, const_pointer str, size_type sCount) {
		auto beg = pBegin() + pos;

		return replace(beg, beg + count, str, str + sCount);
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, const_pointer str, size_type sCount) {
		return replace(first, last, str, str + sCount);
	}
	constexpr container_reference replace(size_type pos, size_type count, const_pointer str) {
		auto beg = pBegin() + pos;

		return replace(beg, beg + count, str, str + traits_type::length(str));
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, const_pointer str) {
		return replace(first, last, str, str + traits_type::length(str));
	}
	constexpr container_reference replace(size_type pos, size_type count, size_type cCount, value_type c) {
		auto beg = pBegin() + pos;

		return replace(beg, beg + count, cCount, c);
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, size_type count, value_type c) {
		auto pos = const_cast<pointer>(first.get());
		
		if (first != last || count != 0) {
			auto info = eraseAndInsertGap(pos, last - first, count);

			if (info.isMemReady)
				for (; count != 0; count--, info.result++) traits_type::assign(*info.result, c);
			else
				for (; count != 0; count--, info.result++) allocator_traits::construct(m_alloc, info.result, c);
		} else return pos;
	}
	template <class It> constexpr container_reference replace(const_iterator first, const_iterator last, It rFirst, It rLast) requires isIteratorValue<It> {
		auto pos = const_cast<pointer>(first.get());
		
		if (first != last || rFirst != rLast) {
			auto info = eraseAndInsertGap(pos, last - first, rLast - rFirst);

			if (info.isMemReady)
				for (; rFirst != rLast; rFirst++, info.result++) traits_type::assign(*info.result, *rFirst);
			else
				for (; rFirst != rLast; rFirst++, info.result++) allocator_traits::construct(m_alloc, info.result, *rFirst);
		}

		return *this;
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, std::initializer_list<value_type> ilist) {
		return replace(first, last, ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference replace(size_type pos, size_type count, const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		auto beg = pBegin() + pos;
		view_type v(sv);

		return replace(beg, beg + count, v.m_begin, v.m_end);
	}
	template <class StringViewLike> constexpr container_reference replace(const_iterator first, const_iterator last, const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);

		return replace(first, last, v.m_begin, v.m_end);
	}
	template <class StringViewLike> constexpr container_reference replace(size_type pos, size_type count, const StringViewLike& sv, size_type vPos, size_type vCount = npos) requires isConvertibleToView<StringViewLike> {
		view_type v(sv);

		if (vPos > sv.size()) throw std::out_of_range("lsd::BasicString::replace(): Position exceded bounds of inserted string view!");

		auto beg = pBegin() + pos;

		return replace(beg, beg + count, v.m_begin + vPos, v.m_begin + vPos + vCount);
	}

	constexpr container_reference append(size_type count, value_type value) {
		auto s = size();
		smartReserve(s + count);
		
		if (smallStringMode())
			for (auto it = (m_short.data + s); count > 0; count--, it++) traits_type::assign(*it, value);
		else {
			for (; count > 0; count--, m_long.end++) allocator_traits::construct(m_alloc, m_long.end, value);
			allocator_traits::construct(m_alloc, m_long.end, value_type { });
		}

		return *this;
	}
	constexpr container_reference append(const_container_reference str) {
		return append(str.begin(), str.end());
	}
	constexpr container_reference append(const_container_reference str, size_type pos, size_type count = npos) {
		auto s = str.size();
		if (pos > s) throw std::out_of_range("lsd::BasicString::append(): Position exceded string bounds!");

		return append(str.begin() + pos, str.begin() + pos + std::min(count, s - pos));
	}
	constexpr container_reference append(const value_type* s, size_type count) {
		return append(s, s + count);
	}
	constexpr container_reference append(const value_type* s) {
		return append(s, s + traits_type::length(s));
	}
	template <class InputIt> constexpr container_reference append(InputIt first, InputIt last) requires isIteratorValue<InputIt> {
		auto count = last - first;
		auto s = size();

		smartReserve(s + count);

		if (smallStringMode()) 
			for (auto it = (m_short.data + s); first != last; first++, it++) traits_type::assign(*it, *first);
		else {
			for (; first != last; first++, m_long.end++) allocator_traits::construct(m_alloc, m_long.end, *first);
			allocator_traits::construct(m_alloc, m_long.end, value_type { });
		}

		return *this;
	}
	constexpr container_reference append(init_list ilist) {
		return append(ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference append(const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		view_type view(sv);
		return append(view.begin(), view.end());
	}
	template <class StringViewLike> constexpr container_reference append(const StringViewLike& sv, size_type pos, size_type count = npos) requires isConvertibleToView<StringViewLike> {
		view_type view(sv);
		auto s = view.size();
		if (pos > s) throw std::out_of_range("lsd::BasicString::append(): Position exceded string view bounds!");

		return append(view.begin() + pos, view.begin() + pos + std::min(count, s - pos));
	}

	constexpr container_reference operator+=(const_container_reference str) {
		return append(str.begin(), str.end());
	}
	constexpr container_reference operator+=(value_type value) {\
		return append(1, value);
	}
	constexpr container_reference operator+=(const value_type* s) {
		return append(s, s + traits_type::length(s));
	}
	constexpr container_reference operator+=(init_list ilist) {
		return append(ilist.begin(), ilist.end());
	}
	template <class StringViewLike> constexpr container_reference operator+=(const StringViewLike& sv) requires isConvertibleToView<StringViewLike> {
		view_type view(sv);
		return append(view.begin(), view.end());
	}

	constexpr container_reference erase(size_type index = 0, size_type count = npos) {
		erase(m_long.begin + index, m_long.begin + index + std::min(count, size() - index));
		return *this;
	}
	constexpr iterator erase(const_iterator pos) {
		assert((pos < end()) && "lsd::BasicString::erase: past-end iterator passed to erase!");

		auto it = m_long.begin + (pos - m_long.begin);

		traits_type::move(it++, it, (m_long.end - it));

		popBack();

		return it;
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		auto it = std::move(last.get(), pEnd(), first.get());

		destructBehind(it);

		return it;
	}

	constexpr void popBack() {
		destructBehind(pEnd() - 1);
	}
	[[deprecated]] constexpr void pop_back() {
		popBack();
	}

	constexpr void clear() {
		if (smallStringMode()) std::fill_n(m_short.data, smallStringMax, value_type { });
		else destructBehind(m_long.begin);
	}

	constexpr size_type find(const_container_reference other, size_type pos = 0) const noexcept {
		return find(other.cStr(), pos, other.size());
	}
	constexpr size_type find(const_pointer s, size_type pos, size_type count) const {
		auto siz = size();
		auto beg = pBegin();

		if (siz >= count && pos <= siz - count)
			for (auto it = beg + pos; it < (pEnd() - count); it++)
				if (traits_type::compare(s, it, count) == 0) return it - beg;

		return npos;
	}
	constexpr size_type find(const_pointer s, size_type pos = 0) const {
		return find(s, pos, traits_type::length(s));
	}
	constexpr size_type find(value_type c, size_type pos = 0) const noexcept {
		return find(std::addressof(c), pos, 1);
	}
	template <class StringViewLike> constexpr size_type find(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return find(v.m_begin, pos, v.size());
	}

	constexpr size_type rfind(const_container_reference other, size_type pos = npos) const noexcept {
		return rfind(other.cStr(), pos, other.size());
	}
	constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const {
		auto siz = size();
		auto beg = pBegin();
		pos = std::min(pos, siz - count);

		if (siz >= count)
			for (auto it = (beg + pos); it != (beg - 1); it--)
				if (traits_type::compare(s, it, count) == 0) return it - beg;

		return npos;
	}
	constexpr size_type rfind(const_pointer s, size_type pos = npos) const {
		return rfind(s, pos, traits_type::length(s));
	}
	constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept {
		return rfind(std::addressof(c), pos, 1);
	}
	template <class StringViewLike> constexpr size_type rfind(const StringViewLike& sv, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return rfind(v.m_begin, pos, v.size());
	}

	constexpr size_type findFirstOf(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_first_of(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos, size_type count) const {
		auto beg = pBegin();

		for (auto it = beg + pos; it < pEnd(); it++) 
			for (auto sIt = s; sIt != (s + count); sIt++)
				if (traits_type::eq(*it, *sIt)) return it - beg;
			
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
	template <class StringViewLike> constexpr size_type findFirstOf(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findFirstOf(v.m_begin, pos, v.size());
	}
	template <class StringViewLike> [[deprecated]] constexpr size_type find_first_of(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findFirstOf(v.m_begin, pos, v.size());
	}

	constexpr size_type findLastOf(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_last_of(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos, size_type count) const {
		auto siz = size();

		if (siz != 0) {
			auto beg = pBegin();
			pos = std::min(pos, siz - 1);

			for (auto it = beg + pos; it > (beg - 1); it--) 
				for (auto sIt = s; sIt != (s + count); sIt++)
					if (traits_type::eq(*it, *sIt)) return it - beg;
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
	template <class StringViewLike> constexpr size_type findLastOf(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findLastOf(v.m_begin, pos, v.size());
	}
	template <class StringViewLike> [[deprecated]] constexpr size_type find_last_of(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findLastOf(v.m_begin, pos, v.size());
	}

	constexpr size_type findFirstNotOf(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstNotOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_container_reference other, size_type pos = 0) const noexcept {
		return findFirstNotOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos, size_type count) const {
		auto beg = pBegin();

		for (auto it = beg + pos; it < pEnd(); it++) {
			auto found = false;
			for (auto sIt = s; sIt != (s + count) && !found; sIt++)
				if (traits_type::eq(*it, *sIt)) found = true;
			if (!found) return it - beg;
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
	template <class StringViewLike> constexpr size_type findFirstNotOf(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findFirstNotOf(v.m_begin, pos, v.size());
	}
	template <class StringViewLike> [[deprecated]] constexpr size_type find_first_not_of(const StringViewLike& sv, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findFirstNotOf(v.m_begin, pos, v.size());
	}

	constexpr size_type findLastNotOf(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastNotOf(other.cStr(), pos, other.size());
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_container_reference other, size_type pos = npos) const noexcept {
		return findLastNotOf(other.cStr(), pos, other.size());
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos, size_type count) const {
		auto siz = size();

		if (siz != 0) {
			auto beg = pBegin();
			pos = std::min(pos, siz - 1);

			for (auto it = beg + pos; it > (beg - 1); it--) {
				auto found = false;
				for (auto sIt = s; sIt != (s + count) && !found; sIt++)
					if (traits_type::eq(*it, *sIt)) found = true;
				if (!found) return it - beg;
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
	template <class StringViewLike> constexpr size_type findLastNotOf(const StringViewLike& sv, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findLastNotOf(v.m_begin, pos, v.size());
	}
	template <class StringViewLike> [[deprecated]] constexpr size_type find_last_not_of(const StringViewLike& sv, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		auto v = view_type(sv);
		return findLastNotOf(v.m_begin, pos, v.size());
	}


	constexpr int compare(const_container_reference str) const noexcept {
		return compare(0, npos, str.cStr(), str.size());
	}
	constexpr int compare(size_type pos, size_type count, const_container_reference str) const {
		return compare(pos, count, str.cStr(), str.size());
	}
	constexpr int compare(size_type pos, size_type count, const_container_reference str, size_type sPos, size_type sCount = npos) const {
		return compare(pos, count, str.cStr() + sPos, std::min(sCount, str.size() - sPos));
	}
	constexpr int compare(const_pointer s) const {
		return compare(0, npos, s, traits_type::length(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s) const {
		return compare(pos, count, s, traits_type::length(s));
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s, size_type sCount) const {
		auto curSiz = size();
		if (pos > curSiz) throw std::out_of_range("lsd::BasicString::compare(): Position exceded string bounds!");

		auto siz = curSiz - pos;
		count = std::min(count, std::min(sCount, siz));

		auto r = traits_type::compare(pBegin() + pos, s, count);
		if (r == 0) {
			if (siz < sCount) return -2;
			else if (siz > sCount) return 2;
			else return r;
		} else return r;
	}
	template <class StringViewLike> constexpr int compare(const StringViewLike& sv) const noexcept(std::is_nothrow_convertible_v<const StringViewLike&, view_type>) requires isConvertibleToView<StringViewLike> {
		return compare(0, npos, sv.m_begin, sv.size());
	}
	template <class StringViewLike> constexpr int compare(size_type pos, size_type count, const StringViewLike& sv) const requires isConvertibleToView<StringViewLike> {
		return compare(pos, count, sv.m_begin, sv.size());
	}
	template <class StringViewLike> constexpr int compare(size_type pos, size_type count, const StringViewLike& sv, size_type sPos, size_type sCount = npos) const requires isConvertibleToView<StringViewLike> {
		return compare(pos, count, sv.m_begin + sPos, std::min(sCount, sv.size() - sPos));
	}

	constexpr bool startsWith(view_type sv) const noexcept	{
		if (sv.size() <= size()) return traits_type::compare(pBegin(), sv.m_begin, sv.size());
		else return false;
	}
	[[deprecated]] constexpr bool starts_with(view_type sv) const noexcept 	{
		return startsWith(sv);
	}
	constexpr bool startsWith(value_type c) const noexcept {
		return traits_type::eq(*pBegin(), c);
	}
	[[deprecated]] constexpr bool starts_with(value_type c) const noexcept {
		return startsWith(c);
	}
	constexpr bool startsWith(const_pointer s) const {
		return startsWith(view_type(s));
	}
	[[deprecated]] constexpr bool starts_with(pointer s) const {
		return startsWith(s);
	}

	constexpr bool endsWith(view_type sv) const noexcept {
		if (sv.size() <= size()) return traits_type::compare(pEnd() - sv.size() - 1, sv.m_begin, sv.size());
		else return false;
	}
	[[deprecated]] constexpr bool ends_with(view_type sv) const noexcept 	{
		return endsWith(sv);
	}
	constexpr bool endsWith(value_type c) const noexcept {
		return traits_type::eq(back(), c);
	}
	[[deprecated]] constexpr bool ends_with(value_type c) const noexcept {
		return endsWith(c);
	}
	constexpr bool endsWith(const_pointer s) const {
		return endsWith(view_type(s));
	}
	[[deprecated]] constexpr bool ends_with(pointer s) const {
		return endsWith(s);
	}

	constexpr bool contains(view_type sv) const noexcept {
		auto siz = size();
		auto beg = pBegin();

		if (siz >= sv.size())
			for (auto it = beg; it != (pEnd() - sv.size() + 1); it++)
				if (traits_type::compare(sv.data(), it, sv.size()) == 0) return true;

		return false;
	}
	constexpr bool contains(value_type c) const noexcept {
		for (auto it = pBegin(); it != pEnd(); it++)
			if (traits_type::eq(*it, c)) return true;

		return false;
	}
	constexpr bool contains(const_pointer s) const {
		return contains(view_type(s));
	}

	constexpr container substr(size_type pos = 0, size_type count = npos) const& {
		return container(*this, pos, count);
	}
	constexpr container substr(size_type pos = 0, size_type count = npos) && {
		return container(std::move(*this), pos, count);
	}

	constexpr size_type copy(pointer dst, size_type count, size_type pos = 0) const {
		auto s = size();
		if (pos > s) throw std::out_of_range("lsd::BasicString::copy(): Position exceded string bounds!");

		count = std::min(count, s - pos);

		traits_type::copy(dst, pBegin() + pos, count);

		return count;
	}

	template <class ReturnType, class Caster, class... Args> [[nodiscard]] ReturnType castTo(Caster caster, std::size_t* pos, Args&&... args) const {
		auto it = pBegin();
		for (; !std::isspace(*it); it++) { }

		if (pos) {
			value_type c { };
			auto cPtr = &c;
			auto res = caster(it, &cPtr, std::forward<Args>(args)...);

			*pos = c;
			return res;
		} else return caster(it, nullptr, std::forward<Args>(args)...);
	}
	template <class CastType, std::size_t Count> [[nodiscard]] static container castFrom(CastType value, const_pointer format) requires(std::is_arithmetic_v<CastType> && (std::is_same_v<value_type, char> || std::is_same_v<value_type, wchar_t>)) {
		value_type buf[Count] = { };
		if constexpr (std::is_same_v<value_type, char>)
			std::snprintf(buf, Count, format, value);
		else if constexpr (std::is_same_v<value_type, wchar_t>)
			std::swprintf(buf, Count, format, value);
		return container(buf);
	}

	[[nodiscard]] constexpr size_type size() const noexcept {
		return smallStringMode() ? smallStringSize() : (m_long.end - m_long.begin);
	}
	[[nodiscard]] constexpr size_type length() const noexcept {
		return smallStringMode() ? smallStringSize() : (m_long.end - m_long.begin);
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return std::min<size_type>(-1, allocator_traits::max_size(m_alloc));
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr size_type capacity() const noexcept {
		return smallStringMode() ? smallStringCap : (m_long.cap - m_long.begin);
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return smallStringMode() ? traits_type::eq(m_short.data[0], value_type { }) : (m_long.begin == m_long.end);
	}

	[[nodiscard]] constexpr pointer data() noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_pointer data() const noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_pointer cStr() const noexcept {
		return pBegin();
	}
	[[deprecated]] [[nodiscard]] constexpr const_pointer c_str() const noexcept {
		return pBegin();
	}

	[[nodiscard]] constexpr operator view_type() const noexcept {
		return view_type(pBegin(), pEnd());
	}
	[[nodiscard]] constexpr operator std_view_type() const noexcept {
		return std_view_type(pBegin(), pEnd());
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_alloc;
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return allocator();
	}

	[[nodiscard]] constexpr const_reference at(size_type index) const {
		if (smallStringMode()) {
			if (index >= smallStringSize()) throw std::out_of_range("lsd::BasicString::at(): Index exceded array bounds!");
			return m_short.data[index];
		} else {
			auto ptr = m_long.begin + index;
			if (ptr >= m_long.end) throw std::out_of_range("lsd::BasicString::at(): Index exceded array bounds!");
			return *ptr;
		}
	}
	[[nodiscard]] constexpr reference at(size_type index) {
		if (smallStringMode()) {
			if (index >= smallStringSize()) throw std::out_of_range("lsd::BasicString::at(): Index exceded array bounds!");
			return m_short.data[index];
		} else {
			auto ptr = m_long.begin + index;
			if (ptr >= m_long.end) throw std::out_of_range("lsd::BasicString::at(): Index exceded array bounds!");
			return *ptr;
		}
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const {
		if (smallStringMode()) {
			return m_short.data[index];
		} else {
			auto ptr = m_long.begin + index;
			assert((ptr < m_long.end) && "lsd::BasicString::operator[]: Index exceded array bounds!");
			return *ptr;
		}
	}
	[[nodiscard]] constexpr reference operator[](size_type index) {
		if (smallStringMode()) {
			return m_short.data[index];
		} else {
			auto ptr = m_long.begin + index;
			assert((ptr < m_long.end) && "lsd::BasicString::operator[]: Index exceded array bounds!");
			return *ptr;
		}
	}

	friend constexpr container operator+(const_container_reference lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, const_pointer rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, value_type rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, std::type_identity_t<view_type> rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_pointer lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(value_type lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(std::type_identity_t<view_type> lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, const_pointer rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, value_type rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, std::type_identity_t<view_type> rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_pointer lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(value_type lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(std::type_identity_t<view_type> lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}

	friend constexpr bool operator==(const_container_reference s1, const_container_reference s2) {
		return s1.compare(s2);
	}

	friend ostream_type& operator<<(ostream_type& stream, const_container_reference string) {
		stream << string.cStr();
		return stream;
	}
	friend istream_type& operator>>(istream_type& stream, const_container_reference string) {
		stream >> string.cStr();
		return stream;
	}

private:
	[[no_unique_address]] allocator_type m_alloc { };

	union {
		Short m_short { };
		Long m_long;
	};

	constexpr bool smallStringMode() const noexcept {
		return bool(1 & m_short.tag);
	}
	constexpr size_type smallStringSize() const noexcept {
		assert(smallStringMode() && "lsd::BasicString::smallStringSize(): BasicString was not a small string!");

		auto it = m_short.data;
		for (; it != (m_short.data + traits_type::length(m_short.data)) && !traits_type::eq(*it, value_type { }); it++) { }

		return it - m_short.data;
	}

	constexpr pointer pBegin() noexcept {
		if (smallStringMode()) return m_short.data;
		else return m_long.begin;
	}
	constexpr const_pointer pBegin() const noexcept {
		if (smallStringMode()) return m_short.data;
		else return m_long.begin;
	}
	constexpr pointer pEnd() noexcept {
		if (smallStringMode()) return m_short.data + smallStringSize();
		else return m_long.end;
	}
	constexpr const_pointer pEnd() const noexcept {
		if (smallStringMode()) return m_short.data + smallStringSize();
		else return m_long.end;
	}

	constexpr void smartReserve(size_type size) noexcept {
		if (smallStringMode() && size > smallStringMax) { // attempts to keep small string mode
			auto newCap = capacity() * 2;
			reserve((newCap < size) ? size : newCap);
		}
	}
	constexpr void resizeAndClear(size_type size) noexcept { // exclusively for hashmap utility
		clear();
		resize(size);
	}

	constexpr EraseAndInsertGapReturnInfo eraseAndInsertGap(pointer position, size_type eraseCount, size_type gapSize) { // does not check for validity of eraseCount or gapSize
		auto oldSize = size();
		auto minReserveCount = oldSize + gapSize - eraseCount + 1;

		if (smallStringMode() && (minReserveCount < smallStringCap)) { // small string mode
			auto moveSrc = position + eraseCount;
			auto moveDst = moveSrc + gapSize;

			traits_type::move(moveDst, moveSrc, pEnd() - moveSrc + 1);

			return { position, true };
		} else {
			auto cap = capacity();

			if (minReserveCount > cap) {
				auto index = position - m_long.begin;

				// reserve memory without constructing new memory, similar to smartReserve()
				auto doubleCap = cap * 2;
				auto reserveCount = (minReserveCount > doubleCap) ? minReserveCount : doubleCap;
				auto oldBegin = std::exchange(m_long.begin, allocator_traits::allocate(m_alloc, reserveCount));
				
				// calculate new pointers
				m_long.end = m_long.begin + minReserveCount;
				m_long.cap = m_long.begin + reserveCount;

				// prepare some iterators for the following parts
				auto it = m_long.begin;
				auto begIt = oldBegin;

				// re-construct the vector in front of pos/position
				for (; it < (m_long.begin + index); it++, begIt++)
					allocator_traits::construct(m_alloc, it, *begIt);
				
				it += gapSize;
				begIt += eraseCount;

				// reconstruct the remaining parts of the s8tring
				for (; it <= m_long.end; it++, begIt++)
					allocator_traits::construct(m_alloc, it, *begIt);

				allocator_traits::deallocate(m_alloc, oldBegin, cap);

				return { m_long.begin + index, false };
			} else {
				auto oldEnd = std::exchange(m_long.end, m_long.end + gapSize - eraseCount);

				auto endIt = m_long.end;
				auto oldEndIt = oldEnd;

				for (; endIt != oldEnd; oldEndIt--, endIt--)
					allocator_traits::construct(m_alloc, endIt, *oldEndIt);

				auto moveSrc = position + eraseCount;
				traits_type::move(position + gapSize, moveSrc, oldEndIt - moveSrc);

				return { position, true };
			}
		}
	}

	template <class It> constexpr void destructBehind(It position) requires isIteratorValue<It> {
		if (smallStringMode())
			for (auto it = m_short.data + smallStringSize(); it != position; it--) traits_type::assign(*it, value_type { });
		else {
			for (; m_long.end != position; m_long.end--) allocator_traits::destroy(m_alloc, m_long.end);
			traits_type::assign(*++m_long.end, value_type { });
		}
	}
};


using String = BasicString<char>;
using WString = BasicString<wchar_t>;
using U8String = BasicString<char8_t>;
using U16String = BasicString<char16_t>;
using U32String = BasicString<char32_t>;


template <class C> struct Hash<BasicString<C>> {
	using string_type = BasicString<C>;

	std::size_t operator()(const string_type& s) const noexcept { // uses the djb2 instead of murmur- or CityHash
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


[[nodiscard]] inline int stoi(const String& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<int>(std::strtol, pos, base);
}
[[nodiscard]] inline int stoi(const WString& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<int>(std::wcstol, pos, base);
}
[[nodiscard]] inline long stol(const String& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long>(std::strtol, pos, base);
}
[[nodiscard]] inline long stol(const WString& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long>(std::wcstol, pos, base);
}
[[nodiscard]] inline long long stoll(const String& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long long>(std::strtoll, pos, base);
}
[[nodiscard]] inline long long stoll(const WString& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long long>(std::wcstoll, pos, base);
}

[[nodiscard]] inline long stoul(const String& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long>(std::strtoul, pos, base);
}
[[nodiscard]] inline long stoul(const WString& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long>(std::wcstoul, pos, base);
}
[[nodiscard]] inline long long stoull(const String& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long long>(std::strtoull, pos, base);
}
[[nodiscard]] inline long long stoull(const WString& str, std::size_t* pos = nullptr, int base = 10) {
	return str.castTo<long long>(std::wcstoull, pos, base);
}

[[nodiscard]] inline float stof (const String& str, std::size_t* pos = nullptr) {
	return str.castTo<float>(std::strtof, pos);
}
[[nodiscard]] inline float stof (const WString& str, std::size_t* pos = nullptr) {
	return str.castTo<float>(std::wcstof, pos);
}
[[nodiscard]] inline double stod (const String& str, std::size_t* pos = nullptr) {
	return str.castTo<double>(std::strtod, pos);
}
[[nodiscard]] inline double stod (const WString& str, std::size_t* pos = nullptr) {
	return str.castTo<double>(std::wcstod, pos);
}
[[nodiscard]] inline long double stold(const String& str, std::size_t* pos = nullptr) {
	return str.castTo<long double>(std::strtold, pos);
}
[[nodiscard]] inline long double stold(const WString& str, std::size_t* pos = nullptr) {
	return str.castTo<long double>(std::wcstold, pos);
}


[[nodiscard]] inline String toString(int value) {
	return String::castFrom<int, SIGNED_SCALAR_DIGITS(int)>(value, "%i");
}
[[nodiscard]] inline String toString(long value) {
	return String::castFrom<long, SIGNED_SCALAR_DIGITS(long)>(value, "%li");
}
[[nodiscard]] inline String toString(long long value) {
	return String::castFrom<long long, SIGNED_SCALAR_DIGITS(long long)>(value, "%lli");
}
[[nodiscard]] inline String toString(unsigned value) {
	return String::castFrom<unsigned, UNSIGNED_SCALAR_DIGITS(unsigned)>(value, "%u");
}
[[nodiscard]] inline String toString(unsigned long value) {
	return String::castFrom<unsigned long, UNSIGNED_SCALAR_DIGITS(unsigned long)>(value, "%lu");
}
[[nodiscard]] inline String toString(unsigned long long value) {
	return String::castFrom<unsigned long long, UNSIGNED_SCALAR_DIGITS(unsigned)>(value, "%llu");
}
[[nodiscard]] inline String toString(float value) {
	return String::castFrom<float, sizeof(float) * 8 + 1>(value, "%g");
}
[[nodiscard]] inline String toString(double value) {
	return String::castFrom<double, sizeof(double) * 8 + 1>(value, "%g");
}
[[nodiscard]] inline String toString(long double value) {
	return String::castFrom<long double, sizeof(long double) * 8 + 1>(value, "%g");
}

[[nodiscard]] inline WString toWString(int value) {
	return WString::castFrom<int, SIGNED_SCALAR_DIGITS(int)>(value, L"%i");
}
[[nodiscard]] inline WString toWString(long value) {
	return WString::castFrom<long, SIGNED_SCALAR_DIGITS(long)>(value, L"%li");
}
[[nodiscard]] inline WString toWString(long long value) {
	return WString::castFrom<long long, SIGNED_SCALAR_DIGITS(long long)>(value, L"%lli");
}
[[nodiscard]] inline WString toWString(unsigned value) {
	return WString::castFrom<unsigned, UNSIGNED_SCALAR_DIGITS(unsigned)>(value, L"%u");
}
[[nodiscard]] inline WString toWString(unsigned long value) {
	return WString::castFrom<unsigned long, UNSIGNED_SCALAR_DIGITS(unsigned long)>(value, L"%lu");
}
[[nodiscard]] inline WString toWString(unsigned long long value) {
	return WString::castFrom<unsigned long long, UNSIGNED_SCALAR_DIGITS(unsigned)>(value, L"%llu");
}
[[nodiscard]] inline WString toWString(float value) {
	return WString::castFrom<float, sizeof(float) * 8 + 1>(value, L"%g");
}
[[nodiscard]] inline WString toWString(double value) {
	return WString::castFrom<double, sizeof(double) * 8 + 1>(value, L"%g");
}
[[nodiscard]] inline WString toWString(long double value) {
	return WString::castFrom<long double, sizeof(long double) * 8 + 1>(value, L"%g");
}

} // namespace lsd
