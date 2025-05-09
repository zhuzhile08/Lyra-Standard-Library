/************************
 * @file Dynarray.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief a (slightly stupid) array masked as a vector
 * 
 * @date 2023-01-14
 * @copyright Copyright (c) 2023
 ************************/

#pragma once

#include "Array.h"
#include "Iterators.h"

#include <type_traits>
#include <stdexcept>

namespace lsd {

template <typename Ty>
concept DynarrayValueType = std::is_move_assignable_v<Ty> && std::is_default_constructible_v<Ty>; // Allowed dynamic array internal type

// Very dangerous dynamic array implementation, please only store contents <= 4 bytes in small quantities
template <DynarrayValueType Ty, std::size_t Capacity> struct Dynarray {
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using value_type = Ty;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using array = Array<value_type, Capacity>;

	using iterator = typename array::iterator;
	using const_iterator = typename array::const_iterator;
	using reverse_iterator = typename array::reverse_iterator;
	using const_reverse_iterator = typename array::const_reverse_iterator;

	using container = Dynarray<value_type, Capacity>;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	constexpr Dynarray() noexcept = default;
	constexpr Dynarray(size_type size) {
		resize(size);
	}
	constexpr Dynarray(size_type size, const Ty& value) {
		assign(size, value);
	}
	template <class It> constexpr Dynarray(It first, It last) requires isIteratorValue<It> {
		assign(first, last);
	}
	constexpr Dynarray(const_container_reference other) : m_size(other.m_size), m_array(other.m_array) { }
	constexpr Dynarray(container_rvreference other) : m_size(std::move(other.m_size)), m_array(std::move(other.m_array)) { }
	constexpr Dynarray(init_list ilist) {
		assign(ilist.begin(), ilist.end());
	}
	template <size_type OtherSize> constexpr Dynarray(const Dynarray<value_type, OtherSize>& other) { 
		assign(other.begin(), other.end());
	}
	template <size_type OtherSize> constexpr Dynarray(Dynarray<value_type, OtherSize>&& other) {
		assign(other.begin(), other.end());
	}


	constexpr container& operator=(const_container_reference other) {
		assign(other.begin(), other.end());
		return *this;
	}
	constexpr container& operator=(container_rvreference other) noexcept {
		m_array = std::move(other.m_array);
		m_size = other.m_size;
		return *this;
	}
	constexpr container& operator=(init_list ilist) {
		assign(ilist.begin(), ilist.end());
		return *this;
	}

	constexpr void assign(size_type count, const_reference value) {
		clear();
		resize(count, value);
	}
	template <class It> constexpr void assign(It first, It last) noexcept requires isIteratorValue<It> {
		size_type newSize = last - first;
		m_size = std::min(newSize, Capacity);
		
		if constexpr (std::is_copy_constructible_v<value_type>) 
			for (auto it = &m_array[0]; first != last; first++, it++) 
				*it = *first;
		else 
			for (auto it = &m_array[0]; first != last; first++, it++) 
				*it = std::move(*first);
	}
	constexpr void assign(init_list ilist) {
		assign(ilist.begin(), ilist.end());
	}

	constexpr void swap(const_container_reference array) noexcept {
		std::swap(m_array, array.m_array);
	}


	[[nodiscard]] constexpr reference front() {
		return *m_array.m_array;
	}
	[[nodiscard]] constexpr const_reference front() const {
		return *m_array.m_array;
	}
	[[nodiscard]] constexpr reference back() {
		return *((m_array.m_array + m_size) - 1);
	}
	[[nodiscard]] constexpr const_reference back() const {
		return *((m_array.m_array + m_size) - 1);
	}

	[[nodiscard]] constexpr iterator begin() noexcept {
		return m_array.m_array;
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return m_array.m_array;
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return m_array.m_array;
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return m_array.m_array + m_size;
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return m_array.m_array + m_size;
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return m_array.m_array + m_size;
	}
	[[nodiscard]] constexpr iterator arrayEnd() noexcept {
		return m_array.m_array + Capacity;
	}
	[[nodiscard]] constexpr const_iterator arrayEnd() const noexcept {
		return m_array.m_array + Capacity;
	}
	[[nodiscard]] constexpr const_iterator carrayEnd() const noexcept {
		return m_array.m_array + Capacity;
	}
	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
		return m_array.m_array + std::max(m_size, 1) - 1;
	}
	[[nodiscard]] constexpr const_iterator rbegin() const noexcept {
		return m_array.m_array + std::max(m_size, 1) - 1;
	}
	[[nodiscard]] constexpr const_iterator crbegin() const noexcept {
		return m_array.m_array + std::max(m_size, 1) - 1;
	}
	[[nodiscard]] constexpr iterator rend() noexcept {
		return m_array.m_array - 1;
	}
	[[nodiscard]] constexpr const_iterator rend() const noexcept {
		return m_array.m_array - 1;
	}
	[[nodiscard]] constexpr const_iterator crend() const noexcept {
		return m_array.m_array - 1;
	}

	constexpr void resize(size_type size) noexcept {
		if (m_size < size) m_size = size;
		else for ( ; m_size > size; m_size--) popBack();
	}
	constexpr void resize(size_type size, const_reference value) noexcept {
		if (m_size < size) {
			for ( ; size != 0; size--) pushBack(value);
			m_size += size;
		}
		else for ( ; m_size > size; m_size--) popBack();
	}


	constexpr iterator insert(const_iterator pos, const_reference value) { 
		if (full()) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		auto prevIt = &back();
		auto moveIt = prevIt + 1;
		auto insertIt = const_cast<value_type*>(&*pos);

		if (m_size != 0)
			for (; prevIt != insertIt - 1; moveIt--, prevIt--)
				*moveIt = std::move(*prevIt);

		++m_size;
		*insertIt = value;

		return insertIt;
	}
	constexpr iterator insert(const_iterator pos, rvreference value) {
		if (full()) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		auto prevIt = &back();
		auto moveIt = prevIt + 1;
		auto insertIt = const_cast<value_type*>(&*pos);

		if (m_size != 0)
			for (; prevIt != insertIt - 1; moveIt--, prevIt--)
				*moveIt = std::move(*prevIt);

		++m_size;
		*insertIt = std::move(value);

		return insertIt;
	}
	constexpr iterator insert(const_iterator pos, size_type count, const_reference value) {
		if (m_size + count > Capacity) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		auto moveIt = &back();
		auto lastIt = moveIt + count;
		auto insertIt = const_cast<value_type*>(&*pos);

		for (; moveIt != insertIt - 1; moveIt--, lastIt--)
			*lastIt = std::move(*moveIt);

		m_size += count;

		for (auto it = insertIt; count > 0; count--, it++) *it = value;

		return insertIt;
	}
	template <class It> constexpr iterator insert(const_iterator pos, It first, It last) requires isIteratorValue<It> {
		auto count = last - first;

		if (m_size + count > Capacity) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		auto moveIt = &back();
		auto lastIt = moveIt + count;
		auto insertIt = const_cast<value_type*>(&*pos);

		for (; moveIt != insertIt - 1; moveIt--, lastIt--)
			*lastIt = std::move(*moveIt);

		m_size += count;

		for (auto it = insertIt; first != last; first++, it++) *it = *first;

		return insertIt;
	}
	constexpr iterator insert(const_iterator pos, init_list ilist) {
		insert(pos, ilist.begin(), ilist.end());
	}

	template <class... Args> constexpr iterator emplace(const_iterator pos, Args&&... args) {
		if (full()) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		auto prevIt = &back();
		auto moveIt = prevIt + 1;
		auto insertIt = const_cast<value_type*>(&*pos);

		if (m_size != 0)
			for (; prevIt != insertIt - 1; moveIt--, prevIt--)
				*moveIt = std::move(*prevIt);

		++m_size;
		new (insertIt) value_type(std::forward<Args>(args)...);

		return insertIt;
	}
	template <class... Args> constexpr reference emplaceBack(Args&&... args) {
		if (full()) throw std::out_of_range("lsd::Dynarray::insert: Dynamic Array is already full!");

		new (&m_array[m_size++]) value_type(std::forward<Args>(args)...);

		return back();
	}
	template <class... Args> [[deprecated]] constexpr reference emplace_back(Args&&... args) {
		return emplaceBack(std::forward<Args>(args)...);
	}


	constexpr void pushBack(const_reference value) noexcept {
		m_array[m_size] = value;
		m_size++;
	}
	constexpr void pushBack(rvreference value) noexcept {
		m_array[m_size] = std::move(value);
		m_size++;
	}
	[[deprecated]] constexpr void push_back(const_reference value) noexcept {
		pushBack(value);
	}
	[[deprecated]] constexpr void push_back(rvreference value) noexcept {
		pushBack(std::move(value));
	}


	constexpr iterator erase(const_iterator pos) {
		auto eraseIt = const_cast<value_type*>(&*pos);
		auto nextIt = eraseIt + 1;
		auto result = eraseIt;

		for (; nextIt != &m_array[m_size]; eraseIt++, nextIt++)
			*eraseIt = std::move(*nextIt);

		--m_size;

		return result;
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		auto eraseIt = const_cast<value_type*>(&*first);
		auto size = last - first;
		auto nextIt = eraseIt + size;
		auto result = eraseIt;

		for (; nextIt != &m_array[m_size]; eraseIt++, nextIt++)
			*eraseIt = std::move(*nextIt);

		m_size -= size;

		return result;
	}

	constexpr void popBack() noexcept {
		back() = std::move(value_type());
		m_size--;
	}
	[[deprecated]] constexpr void pop_back() noexcept {
		popBack();
	}


	constexpr void fill(const_reference value) { 
		std::fill_n(begin(), m_size, value);
	}
	constexpr void clear() {
		m_array.fill(Ty());
		m_size = 0;
	}

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_size;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept { 
		return Capacity;
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return Capacity;
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_size == 0;
	}
	[[nodiscard]] constexpr bool full() const noexcept {
		return Capacity == m_size;
	}
	
	[[nodiscard]] constexpr value_type* data() noexcept { return m_array.data(); }
	[[nodiscard]] constexpr const_value* data() const noexcept { return m_array.data(); }
	[[nodiscard]] constexpr operator value_type*() noexcept { return m_array; }
	[[nodiscard]] constexpr operator const_value* () const noexcept { return m_array; }


	[[nodiscard]] constexpr reference operator[](size_type index) noexcept {
		return m_array[index];
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
		return m_array[index];
	}
	[[deprecated]] [[nodiscard]] constexpr reference at(size_type index) noexcept {
		return m_array.at(index);
	}
	[[deprecated]] [[nodiscard]] constexpr const_reference at(size_type index) const noexcept {
		return m_array.at(index);
	}

private:
	size_type m_size = 0;
	array m_array = array();
};

} // namespace lsd
