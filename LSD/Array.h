/*************************
 * @file Array.h
 * @author zhuzhile08 (zhuzhile08@gmail.com)
 * 
 * @brief a basic array implementation
 * 
 * @date 2022-12-24
 * 
 * @copyright Copyright (c) 2022
 *************************/
#pragma once

#include "Iterators.h"

#include <utility>
#include <algorithm>

namespace lsd {

template <class Ty, std::size_t Size> struct Array {
	static_assert(Size != 0, "lsd::Array: A zero length array is forbidden!");

	using value_type = Ty;
	using const_value = const value_type;
	using rvreference = value_type&&;
	using reference = value_type&;
	using const_reference = const_value&;
	using array = value_type[Size];
	using wrapper = Array<value_type, Size>;

	using iterator = Iterator<value_type>;
	using const_iterator = Iterator<const_value>; 
	using reverse_iterator = ReverseIterator<value_type>;
	using const_reverse_iterator = ReverseIterator<const_value>; 

	constexpr void fill(const_reference value) { 
		std::fill_n(begin(), Size, value);
	}

	constexpr void swap(wrapper& array) {
		std::swap_ranges(begin(), end(), array.begin());
	}

	[[nodiscard]] constexpr reference front() {
		return m_array[0];
	}
	[[nodiscard]] constexpr const_reference front() const {
		return m_array[0];
	}
	[[nodiscard]] constexpr reference back() {
		return m_array[Size - 1];
	}
	[[nodiscard]] constexpr const_reference back() const {
		return m_array[Size - 1];
	}

	[[nodiscard]] constexpr iterator begin() noexcept {
		return &m_array[0];
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return &m_array[0];
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return &m_array[0];
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return &m_array[Size];
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return &m_array[Size];
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return &m_array[Size];
	}
	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
		return (&m_array[Size]) - 1;
	}
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
		return (&m_array[Size]) - 1;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		return (&m_array[Size]) - 1;
	}
	[[nodiscard]] constexpr reverse_iterator rend() noexcept {
		return (&m_array[0]) - 1;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		return (&m_array[0]) - 1;
	}
	[[nodiscard]] constexpr const_iterator crend() const noexcept {
		return (&m_array[0]) - 1;
	}

	[[nodiscard]] constexpr reference operator[](std::size_t index) noexcept {
		assert((index < Size) && "lsd::Array::operator[]: Index exceeded array bounds!");
		return m_array[index];
	}
	[[nodiscard]] constexpr const_reference operator[](std::size_t index) const noexcept {
		assert((index < Size) && "lsd::Array::operator[]: Index exceeded array bounds!");
		return m_array[index];
	}
	[[deprecated]] [[nodiscard]] constexpr reference at(std::size_t index) {
		if (index < Size) throw std::out_of_range("lsd::Array::at: Index exceeded array bounds!");
		return m_array[index];
	}
	[[deprecated]] [[nodiscard]] constexpr const_reference at(std::size_t index) const {
		if (index < Size) throw std::out_of_range("lsd::Array::at: Index exceeded array bounds!");
		return m_array[index];
	}

	[[nodiscard]] constexpr std::size_t size() const noexcept {
		return Size;
	}
	[[nodiscard]] constexpr std::size_t maxSize() const noexcept {
		return std::numeric_limits<std::size_t>::max();
	}
	[[nodiscard]] [[deprecated]] constexpr std::size_t max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return Size == 0;
	}

	[[nodiscard]] constexpr value_type* data() noexcept { return m_array; }
	[[nodiscard]] constexpr const value_type* data() const noexcept { return m_array; }

	array m_array;
};

} // namespace lsd
