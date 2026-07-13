/*************************
 * @file String.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief String implementation
 * 
 * @date 2024-02-24
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/CoreUtility.hpp"
#include "Utility.h"
#include "Iterators.h"
#include "CharTraits.h"
#include "StringView.h"

#include "Detail/String/StringCore.hpp"
#include "Detail/String/BasicStringHash.hpp"
#include "Detail/String/BasicLongString.hpp"
#include "Detail/String/BasicSmallString.hpp"

#include <cstdlib>
#include <cassert>
#include <iterator>
#include <utility>
#include <initializer_list>
#include <ostream>
#include <istream>

namespace lsd {

namespace detail {

// Utility macros

#define LSD_STRING_SIGNED_SCALAR_DIGITS(type) ((sizeof(type) / 2) * 3 + sizeof(type)) + 2
#define LSD_STRING_UNSIGNED_SCALAR_DIGITS(type) ((sizeof(type) / 2) * 3 + sizeof(type)) + 1


#define LSD_STRING_IF_SSO if constexpr (SSOEnabled)
#define LSD_STRING_REQUIRES_SSO requires (SSOEnabled)
#define LSD_STRING_REQUIRES_NOSSO requires (!SSOEnabled)

#define LSD_STRING_IF_SSO_ENABLED LSD_STRING_IF_SSO if (m_short.enabled())
#define LSD_STRING_IF_SSO_FITS(count) LSD_STRING_IF_SSO if (short_type::fits(count))
#define LSD_STRING_IF_SSO_OTHER_ENABLED LSD_STRING_IF_SSO if (other.m_short.enabled())

#define LSD_STRING_IF_SSO_ENABLED_AND_FITS(count) LSD_STRING_IF_SSO_ENABLED { if (short_type::fits(count))
#define LSD_STRING_ELSE_DISABLE_SSO else shortToLong(); }

} // namespace detail


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

	using iterator = ContinuousIterator<value_type>;
	using const_iterator = ContinuousIterator<const_value>; 
	using reverse_iterator = ReverseContinuousIterator<value_type>;
	using const_reverse_iterator = ReverseContinuousIterator<const_value>; 

	using container = BasicString;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	using view_type = BasicStringView<value_type, traits_type>;
	using std_view_type = std::basic_string_view<value_type, traits_type>;
	using ostream_type = std::basic_ostream<value_type, std::char_traits<value_type>>;
	using istream_type = std::basic_istream<value_type, std::char_traits<value_type>>;

	static constexpr size_type npos = string::strNPos;

private:
	using sso_long_string = string::BasicLongString<value_type, traits_type, allocator_type, 1>;

	// SSO calculations

	static constexpr size_type smallStringCapacity = (sizeof(sso_long_string) - alignof(value_type)) / sizeof(value_type);
	static constexpr bool SSOEnabled = smallStringCapacity > 1;


	// String storage type
	
	using long_type = string::BasicLongString<value_type, traits_type, allocator_type, smallStringCapacity>;
	using short_type = std::conditional_t<SSOEnabled,
										  string::BasicSmallString<value_type, traits_type, smallStringCapacity>,
										  std::in_place_t>;


	// Internally, the BasicString makes following promises:
	// 1. If a character and a terminator can be stored within the size of SSOLong, small string optimization is enabled
	//    Else, the container optimizes for size and disables any SSO completely, which means the extra padding is removed
	// 2. When SSO is enabled:
	//    a) The container is EITHER in SSO mode ...
	//    b) ... OR the container always contains valid and allocated memory
	//    c) m_long.m_end is constructed and its value is \0
	//    d) The container will ONLY exit SSO mode if more memory needs to be allocated than SSO can support
	//    e) The container will ONLY reenter SSO mode if requested by shrinkToFit()
	// 3. When SSO is disabled:
	//	  a) The container must not always contain valid memory
	//	  b) m_long.m_end is constructed and its value is \0 IF the container contains valid memory

public:
	static_assert(!std::is_array_v<value_type>, "lsd::String: Character type has to a non-array!");
	static_assert(std::is_trivial_v<value_type>, "lsd::String: Character type has to trivial!");
	static_assert(std::is_standard_layout_v<value_type>, "lsd::String: Character type has to  in standard layout!");
	static_assert(std::is_same_v<value_type, typename traits_type::char_type>, "lsd::String: Character type has to be the same as the type provided to the character traits!");


	constexpr BasicString() noexcept(noexcept(allocator_type())) : BasicString(allocator_type()) { }
	constexpr explicit BasicString(const_alloc_reference alloc) noexcept(noexcept(allocator_type())) : m_alloc(alloc) { }

	constexpr BasicString(size_type count, value_type value, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		LSD_STRING_IF_SSO_FITS(count) {
			m_short.construct(count, value);
			return;
		}

		disableSSO();
		m_long.construct(m_alloc, count, value);
	}

	template <std::input_iterator Iter> constexpr BasicString(Iter first, Iter last, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		constructFromRange(first, last);
	}

	constexpr BasicString(const_pointer s, size_type count, const_alloc_reference alloc = allocator_type())
		 : BasicString(s, s + count, alloc) { }
	constexpr BasicString(const_pointer s, const_alloc_reference alloc = allocator_type())
		 : BasicString(s, s + traits_type::length(s), alloc) { }

	constexpr BasicString(const_container_reference other) : 
		BasicString(other, allocator_traits::select_on_container_copy_construction(other.allocator())) { }
	constexpr BasicString(const_container_reference other, const_alloc_reference alloc) : 
		BasicString(other, 0, alloc) { }
	constexpr BasicString(const_container_reference other, size_type pos, size_type count, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		constructFromString(other, pos, count);
	}
	constexpr BasicString(const_container_reference other, size_type pos, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{	
		constructFromString(other, pos, npos);
	}

	constexpr BasicString(container_rvreference other) noexcept
		: m_alloc(std::exchange(other.m_alloc, m_alloc))
	{
		constructFromString(std::move(other));
	}
	constexpr BasicString(container_rvreference other, const_alloc_reference alloc)
		: m_alloc(alloc)
	{
		constructFromString(std::move(other));
	}
	constexpr BasicString(container_rvreference other, size_type pos, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		if (pos == 0)
			constructFromString(std::move(other));
		else
			constructFromString(other, pos, npos);
	}
	constexpr BasicString(container_rvreference other, size_type pos, size_type count, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		if (pos == 0)
			constructFromString<true>(std::move(other), count);
		else
			constructFromString(other, pos, count);
	}

	constexpr BasicString(init_list ilist, const_alloc_reference alloc = allocator_type())
		: BasicString(ilist.begin(), ilist.end(), alloc) { }

	template <detail::StringViewLike<view_type> View>
	explicit constexpr BasicString(const View& sv, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		const view_type view { sv };
		constructFromRange(view.begin(), view.end());
	}
	template <detail::StringViewLike<view_type> View>
	constexpr BasicString(const View& sv, size_type pos, size_type count, const_alloc_reference alloc = allocator_type())
		: m_alloc(alloc)
	{
		const view_type view { view_type { sv }.substr(pos, count) };
		constructFromRange(view.m_begin, view.m_end);
	}


	constexpr ~BasicString() {
		LSD_STRING_IF_SSO_ENABLED
			return;

		if (m_long.m_begin)
			m_long.dealloc(m_alloc);
	}


	// Assignment operators

	constexpr container_reference operator=(const_container_reference other) {
		if (this == &other)
			return *this;

		if (!assignIfPossible<false>(long_type::propagateAllocatorCopy(m_alloc, other.m_alloc), other))
			m_long.assign(m_alloc, other.m_long.size(), other.m_long.m_begin, other.m_long.m_end);

		return *this;
	}

	constexpr container_reference operator=(container_rvreference other) noexcept {
		if (this == &other)
			return *this;

		if (!assignIfPossible<true>(long_type::reallocOnMove(m_alloc, other.m_alloc), std::move(other)))
			m_long.move(m_alloc, std::move(other.m_long), std::move(other.m_alloc));

		return *this;
	}

	constexpr container_reference operator=(const_pointer s) {
		return assign(s, s + traits_type::length(s));
	}
	constexpr container_reference operator=(value_type c) {
		return assign(size_type { 1 }, c);
	}

	constexpr container_reference operator=(init_list ilist) {
		return assign(ilist.begin(), ilist.end());
	}

	template <detail::StringViewLike<view_type> View> constexpr container_reference operator=(const View& sv) {
		const view_type view { sv };
		return assign(view.begin(), view.end());
	}

	constexpr container_reference operator=(std::nullptr_t) = delete;


	// Manual assignment functions

	constexpr container_reference assign(size_type count, value_type value) {
		assignBasic(count, value);
		return *this;
	}

	template <std::random_access_iterator Iter> constexpr container_reference assign(Iter first, Iter last) {
		const auto dist = std::distance(first, last);
		if (dist == 0)
			clear();
		else
			assignBasic(dist, first, last);

		return *this;

	}

	template <std::random_access_iterator Iter> constexpr container_reference assign(Iter first, Iter last)
		requires(!std::random_access_iterator<Iter>)
	{
		LSD_STRING_IF_SSO_ENABLED {
			if (m_short.assign(first, last))
				return *this;
			else
				disableSSO();
		}

		m_long.assign(m_alloc, first, last);

		return *this;
	}
	
	constexpr container_reference assign(const_pointer s, size_type count) {
		return assign(s, s + count);
	}
	constexpr container_reference assign(const_pointer s) {
		return assign(s, s + traits_type::length(s));
	}

	constexpr container_reference assign(const_container_reference other) {
		if (this == &other)
			return *this;

		const view_type view { other };
		return assign(view.m_begin, view.m_end);
	}
	constexpr container_reference assign(const_container_reference other, size_type pos, size_type count = npos) {
		const view_type view { other.subview(pos, count) };
		return assign(view.m_begin, view.m_end);
	}

	constexpr container_reference assign(container_rvreference other)
		noexcept(allocator_traits::propagate_on_container_move_assignment::value || allocator_traits::is_always_equal::value)
	{
		if constexpr (!allocator_traits::propagate_on_container_move_assignment::value && !allocator_traits::is_always_equal::value)
			if (m_alloc != other.m_alloc)
				return assign(other);

		if (!assignIfSSO(other))
			m_long.move(m_alloc, std::move(other.m_long), std::move(other.m_alloc));

		return *this;
	}

	constexpr container_reference assign(init_list ilist) {
		return assign(ilist.begin(), ilist.end());
	}

	template <detail::StringViewLike<view_type> View> constexpr container_reference assign(const View& sv) {
		const view_type view { sv };
		return assign(view.m_begin, view.m_end);
	}
	template <class StringViewLike> constexpr container_reference assign(const StringViewLike& sv, size_type pos, size_type count = npos) {
		const view_type view { view_type { sv }.substr(pos, count) };
		return assign(view.m_begin, view.m_end);
	}


	constexpr void swap(container_reference other) {
		if (this == &other)
			return;

		const bool differentAlloc = long_type::propagateAllocatorSwap(m_alloc, other.m_alloc);

		LSD_STRING_IF_SSO {
			if (m_short.enabled()) {
				if (other.m_short.enabled()) {
					if constexpr (allocator_traits::propagate_on_container_swap::value) if (differentAlloc)
						std::swap(m_alloc, other.m_alloc);

					m_short.swap(other.m_short);
				} else
					other.swapLongWithShort(differentAlloc, *this);

				return;
			} else if (other.m_short.enabled()) {
				swapLongWithShort(differentAlloc, other);
				return;
			}
		}

		m_long.swap(m_alloc, differentAlloc, other.m_long, other.m_alloc);
	}


	[[nodiscard]] constexpr iterator begin() noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return cpBegin();
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return cpBegin();
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return pEnd();
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return cpEnd();
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return cpEnd();
	}
	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
		auto e = pEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
		auto e = cpEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		auto e = cpEnd();
		return e ? e - 1 : nullptr;
	}
	[[nodiscard]] constexpr reverse_iterator rend() noexcept {
		auto b = pBegin();
		return b ? b - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		auto b = cpBegin();
		return b ? b - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
		auto b = cpBegin();
		return b ? b - 1 : nullptr;
	}

	[[nodiscard]] constexpr reference front() noexcept {
		return *pBegin();
	}
	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *cpBegin();
	}
	[[nodiscard]] constexpr reference back() noexcept {
		return *(pEnd() - 1);
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(cpEnd() - 1);
	}


	// Size and capacity modification functions
	
	constexpr void resize(size_type count) {
		resize(count, value_type { });
	}
	constexpr void resize(size_type count, const_reference value) {
		LSD_STRING_IF_SSO_ENABLED_AND_FITS(count) {
			m_short.resize(count, value);
			return;
		} LSD_STRING_ELSE_DISABLE_SSO

		m_long.resize(m_alloc, count, value);
	}
	constexpr void reserve(size_type count) {
		LSD_STRING_IF_SSO_ENABLED_AND_FITS(count)
			return;
		LSD_STRING_ELSE_DISABLE_SSO

		m_long.reserve(m_alloc, count);
	}
	constexpr void shrinkToFit() {
		LSD_STRING_IF_SSO_ENABLED
			return;

		if (!m_long.shrinkToFit(m_alloc))
			longToShort();
	}
	[[deprecated]] constexpr void shrink_to_fit() {
		shrinkToFit();
	}


	// Index-based insert

	constexpr container_reference insert(size_type index, size_type count, value_type value) {
		return insertBasic(index, count, value);
	}

	template <std::input_iterator Iter> constexpr container_reference insert(size_type index, Iter first, Iter last) {
		const auto dist = std::distance(first, last);
		return insertBasic(index, dist, first, last);
	}
	constexpr container_reference insert(size_type index, const_pointer s) {
		return insert(index, s, s + traits_type::length(s));
	}
	constexpr container_reference insert(size_type index, const_pointer s, size_type count) {
		return insert(index, s, s + count);
	}
	constexpr container_reference insert(size_type index, const_container_reference str) {
		const view_type view { str };
		return insert(index, view.m_begin, view.m_end);
	}
	constexpr container_reference insert(size_type index, const_container_reference str, size_type sIndex, size_type count = npos) {
		const view_type view { str.subview(sIndex, count) };
		return insert(index, view.m_begin, view.m_end);
	}

	// Iterator-based insert

	constexpr iterator insert(const_iterator position, value_type value) {
		return insertBasic<false>(position, 1, value);
	}
	constexpr iterator insert(const_iterator position, size_type count, value_type value) {
		return insertBasic(position, count, value);
	}

	template <std::input_iterator Iter> constexpr iterator insert(const_iterator position, Iter first, Iter last) {
		const auto dist = std::distance(first, last);
		return insertBasic(position, dist, first, last);
	}

	constexpr iterator insert(const_iterator pos, init_list ilist) {
		return insert(pos, ilist.begin(), ilist.end());
	}

	template <detail::StringViewLike<view_type> View>
	constexpr container_reference insert(size_type index, const View& sv) {
		const view_type view { sv };
		return insert(index, view.m_begin, view.m_end);
	}
	template <detail::StringViewLike<view_type> View>
	constexpr container_reference insert(size_type index, const View& sv, size_type svIndex, size_type count = npos) {
		const view_type view { view_type { sv }.substr(svIndex, count) };
		return insert(index, view.m_begin, view.m_end);
	}


	// Append functions
	
	constexpr container_reference append(size_type count, value_type value) {
		return appendBasic(count, value);
	}

	template <std::contiguous_iterator Iter> constexpr container_reference append(Iter first, Iter last) {
		const auto dist = std::distance(first, last);
		return appendBasic(dist, first, last);
	}
	template <std::input_iterator Iter> constexpr container_reference append(Iter first, Iter last)
		requires(!std::contiguous_iterator<Iter>)
	{
		LSD_STRING_IF_SSO_ENABLED {
			if (m_short.append(first, last))
				return *this;
			LSD_STRING_ELSE_DISABLE_SSO

		m_long.append(m_alloc, first, last);
		return *this;
	}

	constexpr container_reference append(const value_type* s) {
		return append(s, s + traits_type::length(s));
	}
	constexpr container_reference append(const value_type* s, size_type count) {
		return append(s, s + count);
	}

	template <detail::StringViewLike<view_type> View> constexpr container_reference append(const View& sv) {
		const view_type view { sv };
		return append(view.m_begin, view.m_end);
	}
	template <detail::StringViewLike<view_type> View> constexpr container_reference append(const View& sv, size_type pos, size_type count = npos) {
		const view_type view { view_type { sv }.substr(pos, count) };
		return append(view.m_begin, view.m_begin);
	}

	constexpr container_reference append(const_container_reference str) {
		const view_type view { str };
		return append(view.m_begin, view.m_end);
	}
	constexpr container_reference append(const_container_reference str, size_type pos, size_type count = npos) {
		const view_type view { str.subview(pos, count) };
		return append(view.m_begin, view.m_end);
	}

	constexpr container_reference append(init_list ilist) {
		return append(ilist.begin(), ilist.end());
	}

	// Appending addition operator

	constexpr container_reference operator+=(const_container_reference str) {
		return append(str);
	}
	constexpr container_reference operator+=(value_type value) {
		pushBack(value);
		return *this;
	}
	constexpr container_reference operator+=(const value_type* s) {
		return append(s, s + traits_type::length(s));
	}
	constexpr container_reference operator+=(init_list ilist) {
		return append(ilist.begin(), ilist.end());
	}
	template <detail::StringViewLike<view_type> View> constexpr container_reference operator+=(const View& sv) {
		const auto view { sv };
		return append(view.m_begin, view.m_end);
	}
 

	// Push back

	constexpr void pushBack(value_type value) {	// Special optimization
		LSD_STRING_IF_SSO_ENABLED_AND_FITS(m_short.size() + 1) {
			m_short.append(value);
			return;
		} LSD_STRING_ELSE_DISABLE_SSO

		m_long.append(m_alloc, value);
	}
	[[deprecated]] constexpr void push_back(value_type value) {
		pushBack(value);
	}


	// Index-based replace functions

	constexpr container_reference replace(size_type pos, size_type count, size_type cCount, value_type value) {
		return replaceBasic(pos, count, cCount, value);
	}
	
	template <std::input_iterator Iter> constexpr container_reference replace(size_type pos, size_type count, Iter first, Iter last) {
		const auto dist = std::distance(first, last);
		return replaceBasic(pos, count, dist, first, last);
	}

	constexpr container_reference replace(size_type pos, size_type count, const_pointer str) {
		return replace(pos, count, str, str + traits_type::length(str));
	}
	constexpr container_reference replace(size_type pos, size_type count, const_pointer str, size_type sCount) {
		return replace(pos, count, str, str + sCount);
	}

	constexpr container_reference replace(size_type pos, size_type count, const_container_reference str) {
		const view_type view { str };
		return replace(pos, count, view.m_begin, view.m_end);
	}
	constexpr container_reference replace(size_type pos, size_type count, const_container_reference str, size_type sPos, size_type sCount = npos) {
		const view_type view { str.subview(sPos, sCount) };
		return replace(pos, count, view.m_begin, view.m_end);
	}

	template <detail::StringViewLike<view_type> View>
	constexpr container_reference replace(size_type pos, size_type count, const View& sv) {
		const view_type view { sv };
		return replace(pos, count, view.m_begin, view.m_end);
	}
	template <detail::StringViewLike<view_type> View>
	constexpr container_reference replace(size_type pos, size_type count, const View& sv, size_type vPos, size_type vCount = npos) {
		const view_type view { view_type { sv }.substr(vPos, vCount) };
		return replace(pos, count, view.m_begin, view.m_end);
	}

	// Iterator-based replace functions

	constexpr container_reference replace(const_iterator first, const_iterator last, size_type count, value_type value) {
		return replaceBasic(first, last, count, value);
	}

	template <std::input_iterator Iter> constexpr container_reference replace(const_iterator first, const_iterator last, Iter rFirst, Iter rLast) {
		const auto dist = std::distance(rFirst, rLast);
		return replaceBasic(first, last, dist, rFirst, rLast);
	}

	constexpr container_reference replace(const_iterator first, const_iterator last, const_pointer str, size_type sCount) {
		return replace(first, last, str, str + sCount);
	}
	constexpr container_reference replace(const_iterator first, const_iterator last, const_pointer str) {
		return replace(first, last, str, str + traits_type::length(str));
	}

	constexpr container_reference replace(const_iterator first, const_iterator last, const_container_reference str) {
		const view_type view { str };
		return replace(first, last, view.m_begin, view.m_end);
	}

	constexpr container_reference replace(const_iterator first, const_iterator last, std::initializer_list<value_type> ilist) {
		return replace(first, last, ilist.begin(), ilist.end());
	}
	template <detail::StringViewLike<view_type> View>
	constexpr container_reference replace(const_iterator first, const_iterator last, const View& sv) {
		const view_type view { sv };
		return replace(first, last, view.m_begin, view.m_end);
	}


	// Index-based erase

	constexpr container_reference erase(size_type index = 0, size_type count = npos) {
		LSD_STRING_IF_SSO_ENABLED {
			const auto siz = m_short.size();
			const auto first = m_short.m_data + index;

			m_short.template erase<true>(first, first + std::min(count, siz - index));

			return *this;
		}

		const auto siz = m_long.size();
		const auto first = m_long.m_begin + index;

		m_long.template erase<true>(first, first + std::min(count, siz - index));

		return *this;
	}

	// Iterator-based erase

	constexpr iterator erase(const_iterator pos) {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.erase(pos.get());

		return m_long.erase(pos.get());
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.template erase<false>(first.get(), last.get());

		return m_long.template erase<false>(first.get(), last.get());
	}

	constexpr void popBack() {
		LSD_STRING_IF_SSO_ENABLED {
			if (!m_short.empty())
				m_short.terminateAt(m_short.size() - 1);

			return;
		}

		if (!m_long.empty())
			m_long.terminateAt(m_long.m_end - 1);
	}
	[[deprecated]] constexpr void pop_back() {
		popBack();
	}

	constexpr void clear() {
		LSD_STRING_IF_SSO_ENABLED {
			m_short.terminateAt(0);
			return;
		}

		m_long.terminateAt(m_long.m_begin);
	}


	constexpr container substr(size_type pos = 0, size_type count = npos) const& {
		return container(*this, pos, count);
	}
	constexpr container substr(size_type pos = 0, size_type count = npos) && {
		return container(std::move(*this), pos, count);
	}

	constexpr view_type subview(size_type pos = 0, size_type count = npos) const {
		return subviewBasic<view_type>(pos, count);
	}
	constexpr view_type stdsubview(size_type pos = 0, size_type count = npos) const {
		return subviewBasic<std_view_type>(pos, count);
	}

	constexpr size_type copy(pointer dst, size_type count, size_type pos = 0) const {
		const auto view = subview(pos, count);
		const auto siz = view.size();
		traits_type::copy(dst, view.m_begin, siz);

		return siz;
	}

	template <class ReturnType, class Caster, class... Args> [[nodiscard]] ReturnType castTo(Caster&& caster, std::size_t* pos, Args&&... args) const {
		const view_type view { *this };
		pointer last { const_cast<pointer>(view.m_end) }; // @todo this should not be nesseceary

		errno = 0;
		auto res = std::invoke(std::forward<Caster>(caster), view.m_begin, &last, std::forward<Args>(args)...);

		if (errno == ERANGE)
			throw std::out_of_range("lsd::BasicString::castTo: Number exceeded type limits!");
		if (view.m_begin == last)
			throw std::invalid_argument("lsd::BasicString::castTo: No valid conversion!"); ///@todo may need more checks

		if (pos)
			*pos = view.size();

		return res;
	}

	template <class CastType, std::size_t Count> [[nodiscard]] static container castFrom(CastType value, const_pointer format) 
		requires(std::is_arithmetic_v<CastType> && (std::is_same_v<value_type, char> || std::is_same_v<value_type, wchar_t>))
	{
		value_type buf[Count] = { };
		if constexpr (std::is_same_v<value_type, char>)
			std::snprintf(buf, Count, format, value);
		else if constexpr (std::is_same_v<value_type, wchar_t>)
			std::swprintf(buf, Count, format, value);
		return container(buf);
	}


	// Observation and search
	
	constexpr int compare(container s) const noexcept {
		return view_type(*this).compare(s);
	}
	constexpr int compare(size_type pos, size_type count, container other) const {
		return subview(pos, count).compare(other);
	}
	constexpr int compare(size_type pos, size_type count, container other, size_type oPos, size_type oCount) const {
		return subview(pos, count).compare(other.subview(oPos, oCount));
	}
	constexpr int compare(const_pointer s) const {
		return view_type(*this).compare(s);
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s) const {
		return subview(pos, count).compare(s);
	}
	constexpr int compare(size_type pos, size_type count, const_pointer s, size_type sCount) const {
		return view_type(*this).compare(pos, count, s, sCount);
	}


	constexpr bool startsWith(container other) const noexcept {
		return view_type(*this).startsWith(other);
	}
	[[deprecated]] constexpr bool starts_with(container other) const noexcept {
		return startsWith(other);
	}	
	constexpr bool startsWith(value_type c) const noexcept {
		return view_type(*this).startsWith(c);
	}
	[[deprecated]] constexpr bool starts_with(value_type c) const noexcept {
		return startsWith(c);
	}
	constexpr bool startsWith(const_pointer s) const {
		return view_type(*this).startsWith(s);
	}
	[[deprecated]] constexpr bool starts_with(const_pointer s) const {
		return startsWith(s);
	}

	constexpr bool endsWith(container other) const noexcept {
		return view_type(*this).endWith(other);
	}
	[[deprecated]] constexpr bool ends_with(container other) const noexcept {
		return endsWith(other);
	}
	constexpr bool endsWith(value_type c) const noexcept {
		return view_type(*this).endWith(c);
	}
	[[deprecated]] constexpr bool ends_with(value_type c) const noexcept {
		return endsWith(c);
	}
	constexpr bool endsWith(const_pointer s) const {
		return view_type(*this).endWith(s);
	}
	[[deprecated]] constexpr bool ends_with(const_pointer s) const {
		return endsWith(s);
	}
	constexpr bool endsWith(const_pointer s, size_type len) const {
		return view_type(*this).endWith(s, len);
	}


	constexpr bool contains(container other) const noexcept {
		return view_type(*this).contains(view_type(other));
	}
	constexpr bool contains(value_type c) const noexcept {
		return view_type(*this).contains(c);
	}
	constexpr bool contains(const_pointer s) const {
		return view_type(*this).contains(s);
	}
	constexpr bool contains(const_pointer s, size_type len) const noexcept {
		return view_type(*this).contains(s, len);
	}

	constexpr size_type find(container other, size_type pos = 0) const noexcept {
		return view_type(*this).find(view_type(other), pos);
	}
	constexpr size_type find(const_pointer s, size_type pos, size_type count) const noexcept {
		return view_type(*this).find(s, pos, count);
	}
	constexpr size_type find(const_pointer s, size_type pos = 0) const {
		return view_type(*this).find(s, pos);
	}
	constexpr size_type find(value_type c, size_type pos = 0) const noexcept {
		return view_type(*this).find(c, pos);
	}

	constexpr size_type rfind(container other, size_type pos = npos) const noexcept {
		return view_type(*this).rfind(view_type(other), pos);
	}
	constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const {
		return view_type(*this).rfind(s, pos, count);
	}
	constexpr size_type rfind(const_pointer s, size_type pos = npos) const {
		return view_type(*this).rfind(s, pos);
	}
	constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept {
		return view_type(*this).rfind(c, pos);
	}

	constexpr size_type findFirstOf(container other, size_type pos = 0) const noexcept {
		return view_type(*this).findFirstOf(view_type(other), pos);
	}
	[[deprecated]] constexpr size_type find_first_of(container other, size_type pos = 0) const noexcept {
		return findFirstOf(other, pos);
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos, size_type count) const {
		return view_type(*this).findFirstOf(s, pos, count);
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos, size_type count) const {
		return findFirstOf(s, pos, count);
	}
	constexpr size_type findFirstOf(const_pointer s, size_type pos = 0) const {
		return view_type(*this).findFirstOf(s, pos);
	}
	[[deprecated]] constexpr size_type find_first_of(const_pointer s, size_type pos = 0) const {
		return findFirstOf(s, pos);
	}
	constexpr size_type findFirstOf(value_type c, size_type pos = 0) const noexcept {
		return view_type(*this).findFirstOf(c, pos);
	}
	[[deprecated]] constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept {
		return findFirstOf(c, pos);
	}

	constexpr size_type findLastOf(container other, size_type pos = npos) const noexcept {
		return view_type(*this).findLastOf(view_type(other), pos);
	}
	[[deprecated]] constexpr size_type find_last_of(container other, size_type pos = npos) const noexcept {
		return findLastOf(other, pos);
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos, size_type count) const {
		return view_type(*this).findLastOf(s, pos, count);
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos, size_type count) const {
		return findLastOf(s, pos, count);
	}
	constexpr size_type findLastOf(const_pointer s, size_type pos = npos) const {
		return view_type(*this).findLastOf(s, pos);
	}
	[[deprecated]] constexpr size_type find_last_of(const_pointer s, size_type pos = npos) const {
		return findLastOf(s, pos);
	}
	constexpr size_type findLastOf(value_type c, size_type pos = npos) const noexcept {
		return view_type(*this).findLastOf(c, pos);
	}
	[[deprecated]] constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept {
		return findLastOf(c, pos);
	}

	constexpr size_type findFirstNotOf(container other, size_type pos = 0) const noexcept {
		return view_type(*this).findFirstNotOf(view_type(other), pos);
	}
	[[deprecated]] constexpr size_type find_first_not_of(container other, size_type pos = 0) const noexcept {
		return findFirstNotOf(other, pos);
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos, size_type count) const {
		return view_type(*this).findFirstNotOf(s, pos, count);
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos, size_type count) const {
		return findFirstNotOf(s, pos, count);
	}
	constexpr size_type findFirstNotOf(const_pointer s, size_type pos = 0) const {
		return view_type(*this).findFirstNotOf(s, pos);
	}
	[[deprecated]] constexpr size_type find_first_not_of(const_pointer s, size_type pos = 0) const {
		return findFirstNotOf(s, pos);
	}
	constexpr size_type findFirstNotOf(value_type c, size_type pos = 0) const noexcept {
		return view_type(*this).findFirstNotOf(c, pos);
	}
	[[deprecated]] constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept {
		return findFirstNotOf(c, pos);
	}

	constexpr size_type findLastNotOf(container other, size_type pos = npos) const noexcept {
		return view_type(*this).findLastNotOf(view_type(other), pos);
	}
	[[deprecated]] constexpr size_type find_last_not_of(container other, size_type pos = 0) const noexcept {
		return findLastNotOf(other, pos);
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos, size_type count) const {
		return view_type(*this).findLastNotOf(s, pos, count);
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos, size_type count) const {
		return findLastNotOf(s, pos, count);
	}
	constexpr size_type findLastNotOf(const_pointer s, size_type pos = npos) const {
		return view_type(*this).findLastNotOf(s, pos);
	}
	[[deprecated]] constexpr size_type find_last_not_of(const_pointer s, size_type pos = npos) const {
		return findLastNotOf(s, pos);
	}
	constexpr size_type findLastNotOf(value_type c, size_type pos = npos) const noexcept {
		return view_type(*this).findLastNotOf(c, pos);
	}
	[[deprecated]] constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept {
		return findLastNotOf(c, pos);
	}


	// Member access functions

	[[nodiscard]] constexpr size_type size() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.size();

		return m_long.size();
	}
	[[nodiscard]] constexpr size_type length() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.size();

		return m_long.size();
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return allocator_traits::max_size(m_alloc);
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr size_type capacity() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.capacity();

		return m_long.capacity();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.empty();

		return m_long.empty();
	}

	[[nodiscard]] constexpr pointer data() noexcept {
		return pBegin();
	}
	[[nodiscard]] constexpr const_pointer data() const noexcept {
		return cpBegin();
	}
	[[nodiscard]] constexpr const_pointer cStr() const noexcept {
		return cpBegin();
	}
	[[deprecated]] [[nodiscard]] constexpr const_pointer c_str() const noexcept {
		return cpBegin();
	}

	[[nodiscard]] constexpr operator view_type() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return view_type(m_short.m_data, m_short.m_data + m_short.size());

		return view_type(m_long.m_begin, m_long.m_end);
	}
	[[nodiscard]] constexpr operator std_view_type() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return std_view_type(m_short.m_data, m_short.m_data + m_short.size());

		return std_view_type(m_long.m_begin, m_long.m_end);
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_alloc;
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return allocator();
	}

	[[nodiscard]] constexpr reference at(size_type index) {
		LSD_STRING_IF_SSO_ENABLED {
			if (index >= m_short.size())
				throw std::out_of_range("lsd::BasicString::at(): Index exceeded bounds!");
	
			return m_short.m_data[index];
		}

		const auto ptr = m_long.m_begin + index;
		if (ptr >= m_long.m_end)
			throw std::out_of_range("lsd::BasicString::at(): Index exceeded bounds!");

		return *ptr;
	}
	[[nodiscard]] constexpr const_reference at(size_type index) const {
		LSD_STRING_IF_SSO_ENABLED {
			if (index >= m_short.size())
				throw std::out_of_range("lsd::BasicString::at(): Index exceeded bounds!");
	
			return m_short.m_data[index];
		}

		const auto* const ptr = m_long.m_begin + index;
		if (ptr >= m_long.m_end)
			throw std::out_of_range("lsd::BasicString::at(): Index exceeded bounds!");

		return *ptr;
	}

	[[nodiscard]] constexpr reference operator[](size_type index) {
		LSD_STRING_IF_SSO_ENABLED {
			assert((index < m_short.size()) && "lsd::BasicString::operator[]: Index exceeded bounds!");
			return m_short.m_data[index];
		}

		const auto ptr = m_long.m_begin + index;
		assert((index < m_long.size()) && "lsd::BasicString::operator[]: Index exceeded bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const {
		LSD_STRING_IF_SSO_ENABLED {
			assert((index < m_short.size()) && "lsd::BasicString::operator[]: Index exceeded bounds!");
			return m_short.m_data[index];
		}

		const auto* const ptr = m_long.m_begin + index;
		assert((ptr < m_long.m_end) && "lsd::BasicString::operator[]: Index exceeded bounds!");
		return *ptr;
	}


	// Friend operators

	friend constexpr container operator+(const_container_reference lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, const_pointer rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, value_type rhs) {
		container value { lhs };
		value.pushBack(rhs);
		return value;
	}
	friend constexpr container operator+(const_container_reference lhs, std::type_identity_t<view_type> rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_pointer lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(value_type lhs, const_container_reference rhs) {
		return container(1, lhs).append(rhs);
	}
	friend constexpr container operator+(std::type_identity_t<view_type> lhs, const_container_reference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, container_rvreference rhs) {
		return container(std::move(lhs)).append(std::move(rhs));
	}
	friend constexpr container operator+(container_rvreference lhs, const_container_reference rhs) {
		return container(std::move(lhs)).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, const_pointer rhs) {
		return container(std::move(lhs)).append(rhs);
	}
	friend constexpr container operator+(container_rvreference lhs, value_type rhs) {
		container value { std::move(lhs) };
		value.pushBack(rhs);
		return value;
	}
	friend constexpr container operator+(container_rvreference lhs, std::type_identity_t<view_type> rhs) {
		return container(std::move(lhs)).append(rhs);
	}
	friend constexpr container operator+(const_container_reference lhs, container_rvreference rhs) {
		return container(lhs).append(rhs);
	}
	friend constexpr container operator+(const_pointer lhs, container_rvreference rhs) {
		return container(lhs).append(std::move(rhs));
	}
	friend constexpr container operator+(value_type lhs, container_rvreference rhs) {
		return container(1, lhs).append(std::move(rhs));
	}
	friend constexpr container operator+(std::type_identity_t<view_type> lhs, container_rvreference rhs) {
		return container(lhs).append(std::move(rhs));
	}

	friend constexpr bool operator==(const_container_reference s1, const_container_reference s2) {
		return s1.compare(s2) == 0;
	}
	friend constexpr bool operator==(const_container_reference s1, const_pointer s2) {
		return s1.compare(s2) == 0;
	}
	friend constexpr auto operator<=>(const_container_reference s1, const_container_reference s2) {
		if constexpr (requires { typename traits_type::comparison_category; }) return static_cast<traits_type::comparison_category>(s1.compare(s2) <=> 0);
		else return static_cast<std::weak_ordering>(s1.compare(s2) <=> 0);
	}
	friend constexpr auto operator<=>(const_container_reference s1, const_pointer s2) {
		if constexpr (requires { typename traits_type::comparison_category; }) return static_cast<traits_type::comparison_category>(s1.compare(s2) <=> 0);
		else return static_cast<std::weak_ordering>(s1.compare(s2) <=> 0);
	}

	friend ostream_type& operator<<(ostream_type& stream, const_container_reference string) {
		stream << string.cStr();
		return stream;
	}
	friend istream_type& operator>>(istream_type& stream, container_reference string) {
		using sentry_type = istream_type::sentry;

		sentry_type sentry { stream };
		if (!sentry) {
			stream.setstate(std::ios_base::failbit);
			return stream;
		}

		string.clear();

		const auto streamWidth = stream.width();
		const auto maxReadCount = (streamWidth == 0) ? string.maxSize() : static_cast<size_type>(streamWidth);

		size_type readCount = 0;
		for (; readCount < maxReadCount; readCount++) {
			const auto next = stream.peek();
			if (traits_type::eqIntType(next, traits_type::eof()))
				break;
			if (std::isspace(traits_type::toCharType(next), stream.getloc()))
				break;

			string.pushBack(stream.get());
		}

		if (readCount == 0)
			stream.setstate(std::ios_base::failbit);

		stream.width(0);

		return stream;
	}


private:
	[[no_unique_address]] allocator_type m_alloc { };

	union {
		[[no_unique_address]] short_type m_short { };
		long_type m_long;
	};


	// Initialization functions

	template<std::random_access_iterator Iter> constexpr void constructFromRange(Iter first, Iter last) {
		const auto dist = std::distance(first, last);

		LSD_STRING_IF_SSO_FITS(dist) {
			m_short.construct(dist, first, last);
			return;
		}

		disableSSO(); 
		m_long.construct(m_alloc, dist, first, last);
	}
	template<std::input_iterator Iter> constexpr void constructFromRange(Iter first, Iter last)
		requires (!std::random_access_iterator<Iter>) {

		LSD_STRING_IF_SSO if (m_short.construct(first, last))
			return;

		disableSSO(); 
		m_long.construct(m_alloc, first, last);
	}

	constexpr void constructFromString(const_container_reference other, size_type pos, size_type count) {
		const view_type view { other.subview(pos, count) };
		const auto siz = view.size();

		LSD_STRING_IF_SSO_FITS(siz) {
			m_short.construct(siz, view.m_begin, view.m_end);
			return;
		}

		disableSSO(); 
		m_long.construct(m_alloc, siz, view.m_begin, view.m_end);
	}

	template <bool Terminate = false>
	constexpr void constructFromString(container_rvreference other, size_type count = npos) {
		LSD_STRING_IF_SSO_OTHER_ENABLED {
			m_short.template construct<Terminate>(std::move(other.m_short), count);
			return;
		}

		disableSSO(); 
		m_long.template construct<Terminate>(m_alloc, std::move(other.m_long), other.m_alloc, count);
	}


	// Swapping

	constexpr void swapLongWithShort(bool differentAlloc, container_reference other) {
		// m_long is active, whilst other.m_short is active

		long_type data { std::move(m_long) };
		m_short.construct(other.m_short.size(), other.m_short.m_data, other.m_short.m_data + other.m_short.size());
		other.m_short.disable();

		if constexpr (!allocator_traits::propagate_on_container_swap::value) if (differentAlloc) { // We need to allocate a new bufer for other, and retrieve its short
			other.m_long.construct(m_alloc, data.size(), data.m_begin, data.m_end);
			data.dealloc(m_alloc);

			return;
		}

		// If the allocators are the same or if they should be propagated, then the buffers can just be swapped
		if constexpr (allocator_traits::propagate_on_container_swap::value) if (differentAlloc)
			std::swap(m_alloc, other.m_alloc);

		other.m_long = std::move(data);
	}

	// Move and assignment

	template <class... Args> constexpr void assignBasic(size_type count, Args&&... args) {
		LSD_STRING_IF_SSO_ENABLED {
			if (short_type::fits(count)) {
				m_short.assign(count, std::forward<Args>(args)...);
				return;
			} else
				disableSSO();
		}

		m_long.assign(m_alloc, count, std::forward<Args>(args)...);
	}

 	constexpr bool assignIfSSO(const_container_reference other) {
		LSD_STRING_IF_SSO {
			if (other.m_short.enabled() || m_short.enabled() && short_type::fits(other.m_long.size())) {
				const view_type view { other };
				assign(view.m_begin, view.m_end);

				return true;
			} else if (m_short.enabled()) {
				disableSSO();

				return false;
			}
		}

		return false;
	}

	template <bool Move = true>
	constexpr bool assignIfPossible(bool propagate, std::conditional_t<Move, container_rvreference, const_container_reference> other) {
		LSD_STRING_IF_SSO {
			if (propagate) {
				m_long.template propagateAlloc<Move>(m_short.enabled(), m_alloc, other.m_alloc);
				m_short.enable();
			}

			return assignIfSSO(other);
		} else if (propagate)
			m_long.template propagateAlloc<Move>(m_short.enabled(), m_alloc, other.m_alloc);
	}


	// Basic container content modification

	template <class... Args> constexpr container_reference insertBasic(size_type index, size_type count, Args&&... args) {
		if (count == 0)
			return *this;

		LSD_STRING_IF_SSO_ENABLED_AND_FITS(m_short.size() + count) {
			m_short.template insert<true>(m_short.m_data + index, count, std::forward<Args>(args)...);
			return *this;
		} LSD_STRING_ELSE_DISABLE_SSO

		m_long.template insert<true>(m_alloc, m_long.m_begin + index, count, std::forward<Args>(args)...);
		return *this;
	}
	template <bool CheckCount = true, class... Args> constexpr pointer insertBasic(const_iterator position, size_type count, Args&&... args) {
		if constexpr (CheckCount) if (count == 0)
			return const_cast<pointer>(position.get());

		LSD_STRING_IF_SSO_ENABLED_AND_FITS(m_short.size() + count)
			return m_short.template insert<true>(position.get(), count, std::forward<Args>(args)...);
		LSD_STRING_ELSE_DISABLE_SSO

		return m_long.template insert<true>(m_alloc, position.get(), count, std::forward<Args>(args)...);
	}

	template <class... Args> constexpr container_reference appendBasic(size_type count, Args&&... args) {
		if (count == 0)
			return *this;

		LSD_STRING_IF_SSO_ENABLED_AND_FITS(m_short.size() + count) {
			m_short.append(count, std::forward<Args>(args)...);
			return *this;
		} LSD_STRING_ELSE_DISABLE_SSO

		m_long.append(m_alloc, count, std::forward<Args>(args)...);
		return *this;
	}

	template <class... Args>
	constexpr container_reference replaceBasic(size_type pos, size_type count, size_type insertCount, Args&&... args) {
		if (insertCount == 0)
			return *this;

		LSD_STRING_IF_SSO_ENABLED {
			const auto siz = m_short.size();
			if (short_type::fits(siz - count + insertCount)) {
				const auto first = m_short.m_data + pos;
				m_short.template replace<true>(count, first, first + count, insertCount, std::forward<Args>(args)...);

				return *this;
			} LSD_STRING_ELSE_DISABLE_SSO

		const auto first = m_long.m_begin + pos;
		m_long.template replace<true>(m_alloc, count, first, first + count, insertCount, std::forward<Args>(args)...);

		return *this;
	}
	template <class... Args>
	constexpr container_reference replaceBasic(const_iterator first, const_iterator last, size_type insertCount, Args&&... args) {
		if (insertCount == 0)
			return *this;

		const auto dist = std::distance(first, last);

		LSD_STRING_IF_SSO_ENABLED {
			const auto siz = m_short.size();
			if (short_type::fits(siz - dist + insertCount)) {
				m_short.template replace<false>(dist, first.get(), last.get(), insertCount, std::forward<Args>(args)...);

				return *this;
			} LSD_STRING_ELSE_DISABLE_SSO

		m_long.template replace<false>(m_alloc, dist, first.get(), last.get(), insertCount, std::forward<Args>(args)...);

		return *this;
	}


	// Generalized string range getter functions

	constexpr pointer pBegin() noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.m_data;
		return m_long.m_begin;
	}
	constexpr const_pointer cpBegin() const noexcept {
		LSD_STRING_IF_SSO_ENABLED
			return m_short.m_data;
		return m_long.m_begin;
	}
	constexpr pointer pEnd() noexcept {
		LSD_STRING_IF_SSO_ENABLED {
			return m_short.m_data + m_short.size();
		} return m_long.m_end;
	}
	constexpr const_pointer cpEnd() const noexcept {
		LSD_STRING_IF_SSO_ENABLED {
			return m_short.m_data + m_short.size();
		} return m_long.m_end;
	}

	template <class ViewType>
	constexpr ViewType subviewBasic(size_type pos = 0, size_type count = npos) const {
		LSD_STRING_IF_SSO_ENABLED {
			const auto siz = m_short.size();
			if (pos > siz)
				throw std::out_of_range("lsd::BasicString::subview()/stdsubview(): Out of range position!");

			const auto first = m_short.m_data + pos;
			return ViewType(first, first + std::min(count, siz - pos));
		}

		const auto siz = m_long.size();
		if (pos > siz)
			throw std::out_of_range("lsd::BasicString::subview()/stdsubview(): Out of range position!");

		const auto first = m_long.m_begin + pos;
		return ViewType(first, first + std::min(count, siz - pos));
	}


	// Conversion functions
	
	constexpr void shortToLong() {
		long_type data { };
		const auto siz = m_short.size();
		data.construct(m_alloc, siz, m_short.m_data, m_short.m_data + siz);
	
		m_short.disable();
		m_long = std::move(data);
	}

	constexpr void longToShort() {
		long_type data { std::move(m_long) };
		const auto siz = data.size();

		m_short.construct(siz, data.m_begin, data.m_end);
		data.dealloc(m_alloc);
	}

	constexpr void disableSSO() {
		LSD_STRING_IF_SSO_ENABLED {
			m_short.disable();
			m_long = nullptr;
		}
	}


	friend class Hash<BasicString>;
};


using String = BasicString<char>;
using WString = BasicString<wchar_t>;
using U8String = BasicString<char8_t>;
using U16String = BasicString<char16_t>;
using U32String = BasicString<char32_t>;


inline namespace literals {

inline namespace string_literals {

constexpr String operator""_s(const char* str, std::size_t len) {
	return String { str, len };
}
constexpr U8String operator""_s(const char8_t* str, std::size_t len) {
	return U8String { str, len };
}
constexpr U16String operator""_s(const char16_t* str, std::size_t len) {
	return U16String { str, len };
}
constexpr U32String operator""_s(const char32_t* str, std::size_t len) {
	return U32String { str, len };
}
constexpr WString operator""_s(const wchar_t* str, std::size_t len) {
	return WString { str, len };
}

} // Inline namespace literals

} // Inline namespace string_literals


template <class Char> struct Hash<BasicString<Char>> {
	using string_type = BasicString<Char>;
	using view_type = BasicStringView<Char>;

	constexpr std::size_t operator()(const string_type& s) const noexcept { // Uses the djb2 instead of murmur- or CityHash
		return Hash<view_type>()(view_type(s));
	}

	constexpr std::size_t operator()(const view_type& v) const noexcept {
		return Hash<view_type>()(v);
	}

	constexpr std::size_t operator()(const Char* str) const noexcept {
		return Hash<const Char*>()(str);
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
	return String::castFrom<int, LSD_STRING_SIGNED_SCALAR_DIGITS(int)>(value, "%i");
}
[[nodiscard]] inline String toString(long value) {
	return String::castFrom<long, LSD_STRING_SIGNED_SCALAR_DIGITS(long)>(value, "%li");
}
[[nodiscard]] inline String toString(long long value) {
	return String::castFrom<long long, LSD_STRING_SIGNED_SCALAR_DIGITS(long long)>(value, "%lli");
}
[[nodiscard]] inline String toString(unsigned value) {
	return String::castFrom<unsigned, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned)>(value, "%u");
}
[[nodiscard]] inline String toString(unsigned long value) {
	return String::castFrom<unsigned long, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned long)>(value, "%lu");
}
[[nodiscard]] inline String toString(unsigned long long value) {
	return String::castFrom<unsigned long long, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned long long)>(value, "%llu");
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
	return WString::castFrom<int, LSD_STRING_SIGNED_SCALAR_DIGITS(int)>(value, L"%i");
}
[[nodiscard]] inline WString toWString(long value) {
	return WString::castFrom<long, LSD_STRING_SIGNED_SCALAR_DIGITS(long)>(value, L"%li");
}
[[nodiscard]] inline WString toWString(long long value) {
	return WString::castFrom<long long, LSD_STRING_SIGNED_SCALAR_DIGITS(long long)>(value, L"%lli");
}
[[nodiscard]] inline WString toWString(unsigned value) {
	return WString::castFrom<unsigned, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned)>(value, L"%u");
}
[[nodiscard]] inline WString toWString(unsigned long value) {
	return WString::castFrom<unsigned long, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned long)>(value, L"%lu");
}
[[nodiscard]] inline WString toWString(unsigned long long value) {
	return WString::castFrom<unsigned long long, LSD_STRING_UNSIGNED_SCALAR_DIGITS(unsigned long long)>(value, L"%llu");
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
template <EnumType Enum> [[nodiscard]] inline String toString(Enum e) {
	return lsd::toString(static_cast<std::underlying_type_t<Enum>>(e));
}
template <EnumType Enum> [[nodiscard]] inline WString toWString(Enum e) {
	return lsd::toString(static_cast<std::underlying_type_t<Enum>>(e));
}


template <class CharTy, class Traits, class Alloc> auto quoted(const lsd::BasicString<CharTy, Traits, Alloc>& str, CharTy delim = CharTy('"'), CharTy escape = CharTy('\\')) {
	return quoted(str.data(), delim, escape);
}


// Templated strncmp

template <std::input_iterator Iter> bool strncmp(Iter first, const char* second, std::size_t size) {
	for (; size > 0; size--, first++, second++) {
		if (*first != *second) return false;
	}

	return true;
}

} // namespace lsd
