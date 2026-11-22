/************************
 * @file BasicSmallString.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief SSO-optimized string implementation
 * 
 * @date 2026-06-21
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "../CoreUtility.hpp"

#include <utility>
#include <stdexcept>
#include <algorithm>


namespace lsd {

namespace string {

template <class CharType, class TraitsType, std::size_t Size> class BasicSmallString {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using traits_type = TraitsType;

	using value_type = CharType;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using pointer = value_type*;
	using const_pointer = const_value*;

	using container = BasicSmallString;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

private:
	using data_type = value_type[Size];
	using tag_type = std::uint8_t;

public:
	constexpr BasicSmallString() noexcept = default;

	constexpr BasicSmallString(container_rvreference other) : m_size(other.m_size), m_data(other.m_data) { }
	constexpr BasicSmallString(const_container_reference other) : m_size(other.m_size), m_data(other.m_data) { }


	constexpr void construct(size_type count, value_type value) {
		if (m_size = 0; count != 0) {
			pushBackValues(count, value);
			createTerminator();
		}
	}

	template <std::random_access_iterator Iter> constexpr void construct(size_type distance, Iter first, Iter last) {
		if (m_size = 0; distance != 0) {
			copyFromRange(distance, first);
			createTerminator();
		}
	}

	template <std::input_iterator Iter> constexpr bool construct(Iter first, Iter last)
		requires (!std::random_access_iterator<Iter>)
	{
		for (m_size = 0; first != last; first++)
			if (!append<false>(*first))
				return false;
		
		createTerminator();

		return true;
	}

	template <bool Terminate>
	constexpr void construct(container_rvreference other, size_type count) {
		m_size = 0;

		if constexpr (!Terminate)
			count = other.size();
	
		m_size = static_cast<tag_type>(count);
		traits_type::copy(m_data, other.m_data, count);
		createTerminator();
	}

	
	constexpr void assign(const_container_reference other) {
		m_size = other.m_size;
		traits_type::copy(m_data, other.m_data, size());
	}

	template <std::input_iterator Iter> constexpr void assign(size_type distance, Iter first, Iter last) {
		m_size = 0;
		copyFromRange(distance, first);
		createTerminator();
	}
	template <std::input_iterator Iter> constexpr bool assign(Iter first, Iter last) {
		for (m_size = 0; first != last; first++)
			if (!append<false>(*first))
				return false;

		createTerminator();

		return true;
	}

	constexpr void assign(size_type count, value_type value) {
		if (count == 0) {
			terminateAt(0);
			return;
		}
		
		m_size = 0;
		pushBackValues(count, value);
		createTerminator();
	}


	constexpr void swap(container_reference other) {
		std::swap(m_size, other.m_size);

		const auto end = m_data + std::max(size(), other.size()) + 1;
		for (auto it = m_data, oIt = other.m_data; it != end; it++, oIt++) {
			const auto tmp { *it };
			traits_type::assign(*it, *oIt);
			traits_type::assign(*oIt, tmp);
		}
	}


	constexpr bool resize(size_type newSize, value_type value) noexcept {
		const auto siz = size();
		
		if (siz < newSize) {
			if (!fits(newSize))
				return false;

			pushBackValues(newSize - siz, value);
		} else if (siz > newSize)
			terminateAt(newSize);
		
		return true;
	}


	template <bool ThrowException = false>
	constexpr pointer insert(const_pointer pos, size_type count, value_type value) {
		if constexpr (!ThrowException)
			assert((pos - m_data <= size()) && "lsd::BasicString::insert(): Out of range position!");
		else if (pos - m_data > size())
			throw std::out_of_range("lsd::BasicString::insert(): Out of range position!");

		auto p = const_cast<pointer>(pos);
		return insertGap(0, p, p, &BasicSmallString::pushBackValues, count, value);
	}

	template <bool ThrowException = false, std::input_iterator Iter>
	constexpr pointer insert(const_pointer pos, size_type distance, Iter begin, Iter end) {
		if constexpr (!ThrowException)
			assert((pos - m_data <= size()) && "lsd::BasicString::insert(): Out of range position!");
		else if (pos - m_data > size())
			throw std::out_of_range("lsd::BasicString::insert(): Out of range position!");

		const auto p = const_cast<pointer>(pos);
		return insertGap(0, p, p, &BasicSmallString::copyFromRange<Iter>, distance, begin);
	}

	template <bool CreateTerminator = true> constexpr bool append(value_type value) {
		if (size() == capacity())
			return false;
	
		traits_type::assign(*(m_data + m_size++), value);
		if constexpr (CreateTerminator)
			createTerminator();

		return true;
	}

	constexpr void append(size_type count, value_type value) {
		pushBackValues(count, value);
		createTerminator();
	}

	template <std::contiguous_iterator Iter> constexpr void append(size_type distance, Iter begin, Iter end) {
		copyFromRange(distance, begin);
		createTerminator();
	}
	template <std::input_iterator Iter> constexpr bool append(Iter begin, Iter end)
		requires (!std::contiguous_iterator<Iter>)
	{
		for (; begin != end; begin++)
			if (!append<false>(*begin))
				return false;

		createTerminator();

		return true;
	}

	template <bool ThrowException = false>
	constexpr void replace(size_type eraseCount, const_pointer first, const_pointer last, size_type count, value_type value) {
		assert((last - m_data <= size() && first >= m_data && first <= last) && "lsd::BasicString::replace(): past-end and/or invalid range!");

		if constexpr (ThrowException) if (first - m_data > size())
			throw std::out_of_range("lsd::BasicString::replace(): Out of range position!");

		insertGap(eraseCount, const_cast<pointer>(first), const_cast<pointer>(last), &BasicSmallString::pushBackValues, count, value);
	}

	template <bool ThrowException = false, std::input_iterator Iter>
	constexpr void replace(size_type eraseCount, const_pointer first, const_pointer last, size_type distance, Iter begin, Iter end) {
		assert((last - m_data <= size() && first >= m_data && first <= last) && "lsd::BasicString::replace(): past-end and/or invalid range!");

		if constexpr (ThrowException) if (first - m_data > size())
			throw std::out_of_range("lsd::BasicString::replace(): Out of range position!");

		insertGap(eraseCount, const_cast<pointer>(first), const_cast<pointer>(last), &BasicSmallString::copyFromRange<Iter>, distance, begin);
	}


	constexpr pointer erase(const_pointer pos) {
		const auto offset = pos - m_data;
		assert((offset < size()) && "lsd::BasicString::erase: past-end iterator!");

		auto dst = const_cast<pointer>(pos);

		traits_type::move(dst, dst + 1, --m_size - offset);
		createTerminator();

		return dst;
	}

	template <bool ThrowException> constexpr pointer erase(const_pointer first, const_pointer last) {
		const auto offset = first - m_data;
		assert((first >= m_data && first <= last && (last - m_data) <= size()) && "lsd::BasicString::erase: past-end and/or invalid range!");

		if constexpr (ThrowException) if (offset > size())
			throw std::out_of_range("lsd::BasicString::erase: past-end position!");

		auto dst = const_cast<pointer>(first);
		if (first == last)
			return dst;

		traits_type::move(dst, const_cast<pointer>(last), (m_size -= std::distance(first, last)) - offset);
		createTerminator();

		return dst;
	}


	constexpr void enable() {
		m_size = 0;
	}
	constexpr void disable() {
		m_size = Size;
	}

	constexpr bool enabled() const {
		return m_size != Size;
	}


	constexpr void terminateAt(size_type newSize) {
		m_size = static_cast<tag_type>(newSize);
		createTerminator();
	}


	[[nodiscard]] constexpr size_type size() const noexcept {
		return implicitCast<size_type>(m_size);
	}
	[[nodiscard]] static constexpr size_type capacity() noexcept {
		return Size - 1;
	}
	[[nodiscard]] static constexpr bool fits(size_type siz) noexcept {
		return siz < Size;
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_size == 0;
	}

private:
	tag_type m_size { 0 };
	data_type m_data { };


	// Container content modification

	template <std::input_iterator Iter> constexpr void copyFromRange(size_type distance, Iter first) {
		traits_type::copy(m_data + size(), first, distance);
		m_size += distance;
	}

	constexpr void pushBackValues(size_type count, value_type value) {
		traits_type::assign(m_data + size(), count, value);
		m_size += count;
	}

	constexpr void createTerminator() {
		traits_type::assign(*(m_data + size()), value_type { });
	}


	template <class Inserter, class... Args>
	constexpr pointer insertGap(size_type eraseCount,
								pointer first,
								pointer last,
								Inserter&& inserter,
								size_type insertCount,
								Args&&... args) {
		const auto oldSize = size();
		const auto newSize = oldSize + insertCount - eraseCount;

		const auto oldEnd = m_data + oldSize;
		const auto newEnd = m_data + newSize;

		traits_type::move(first + insertCount, last, std::distance(last, oldEnd) + 1);

		m_size = first - m_data;
		std::invoke(std::forward<Inserter>(inserter), this, insertCount, std::forward<Args>(args)...);
		m_size = newSize;

		return first;
	}


	template <class, class, class> friend class ::lsd::BasicString;
};

} // namespace string

} // namespace lsd
