/*************************
 * @file Vector.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Vector implementation
 * 
 * @date 2024-02-24
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/CoreUtility.h"
#include "Iterators.h"

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <new>
#include <memory>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>

namespace lsd {

template <class Ty, class Alloc = std::allocator<Ty>> class Vector { // @todo custom compile time allocator implementation
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using allocator_type = Alloc;
	using const_alloc_reference = const allocator_type&;
	using allocator_traits = std::allocator_traits<allocator_type>;

	using value_type = Ty;
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

	using container = Vector;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	constexpr Vector() noexcept { }
	constexpr explicit Vector(const_alloc_reference alloc) : m_alloc(alloc) { }
	constexpr Vector(size_type count, const_reference value, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc) {
		resize(count, value);
	}
	constexpr explicit Vector(size_type count, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc) {
		resize(count);
	}
	template <class It> constexpr Vector(It first, It last, const_alloc_reference alloc = allocator_type()) requires isIteratorValue<It> : 
		m_alloc(alloc) {
		if (first != last) {
			auto count = last - first;
			smartReserve(count);

			for (; first != last; first++, m_end++) allocator_traits::construct(m_alloc, m_end, std::move(*first));
		}
	}
	constexpr Vector(const_container_reference other) : Vector(other.m_begin, other.m_end) { }
	constexpr Vector(const_container_reference other, const_alloc_reference alloc) : Vector(other.m_begin, other.m_end, alloc) { }
	constexpr Vector(container_rvreference other) noexcept :
		m_alloc(std::exchange(other.m_alloc, m_alloc)), 
		m_begin(std::exchange(other.m_begin, pointer { })),
		m_end(std::exchange(other.m_end, pointer { })),
		m_cap(std::exchange(other.m_cap, pointer { })) { }
	constexpr Vector(container_rvreference other, const_alloc_reference alloc) {
		if (detail::allocatorPropagationNecessary(other.m_alloc, alloc))
			moveAssign(other.m_begin, other.m_end, other.m_alloc);
		else {
			m_alloc = alloc;
			std::swap(other.m_begin, m_begin);
			std::swap(other.m_end, m_end);
			std::swap(other.m_cap, m_cap);
		}
	}
	constexpr Vector(init_list ilist, const_alloc_reference alloc = allocator_type()) : Vector(ilist.begin(), ilist.end(), alloc) { }

	constexpr ~Vector() {
		if (m_begin) {
			for (auto it = m_begin; it != m_end; it++) allocator_traits::destroy(m_alloc, it);
			allocator_traits::deallocate(m_alloc, m_begin, m_cap - m_begin);
			
			m_begin = nullptr;
			m_end = nullptr;
			m_cap = nullptr;
		}
	}

	constexpr container_reference operator=(const_container_reference other) {
		clear();
		moveAssign(other.m_begin, other.m_end, other.m_alloc);
		return *this;
	}
	constexpr container_reference operator=(container_rvreference other) noexcept {
		if (detail::allocatorPropagationNecessary(other.m_alloc, m_alloc)) {
			clear();
			moveAssign(other.m_begin, other.m_end, other.m_alloc);
		} else {
			std::swap(other.m_alloc, m_alloc);
			std::swap(other.m_begin, m_begin);
			std::swap(other.m_end, m_end);
			std::swap(other.m_cap, m_cap);
		}

		return *this;
	}
	constexpr container_reference operator=(init_list ilist) {
		assign(ilist.begin(), ilist.end());
		return *this;
	}

	constexpr void assign(size_type count, const_reference value) {
		clear();
		resize(count, value);
	}
	template <class It> constexpr void assign(It first, It last) requires isIteratorValue<It> {
		clear();

		if (first != last) {
			auto count = last - first;
			smartReserve(count);

			for (; first != last; m_end++, first++) allocator_traits::construct(m_alloc, m_end, *first);
		}
	}
	constexpr void assign(init_list ilist) {
		assign(ilist.begin(), ilist.end());
	}

	constexpr void swap(container_reference other) {
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
		std::swap(m_cap, other.m_cap);
	}

	[[nodiscard]] constexpr iterator begin() noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return m_end;
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return m_end;
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return m_end;
	}
	[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
		return m_end ? m_end - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
		return m_end ? m_end - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
		return m_end ? m_end - 1 : nullptr;
	}
	[[nodiscard]] constexpr reverse_iterator rend() noexcept {
		return m_begin ? m_begin - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
		return m_begin ? m_begin - 1 : nullptr;
	}
	[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
		return m_begin ? m_begin - 1 : nullptr;
	}

	[[nodiscard]] constexpr reference front() noexcept {
		return *m_begin;
	}
	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *m_begin;
	}
	[[nodiscard]] constexpr reference back() noexcept {
		return *(m_end - 1);
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(m_end - 1);
	}
	
	constexpr void resize(size_type count) {
		auto s = size();
		if (count > s) {
			smartReserve(size() + count);

			for (; count > 0; count--, m_end++) allocator_traits::construct(m_alloc, m_end);
		} else if (count < s) {
			auto pos = m_begin + count + 1;
			for (auto it = pos; it != m_end; it++) allocator_traits::destroy(m_alloc, m_begin);
			m_end = pos;
		}
	}
	constexpr void resize(size_type count, const_reference value) {
		auto s = size();
		if (count > s)
			append(count - s, value);
		else if (count < s) {
			auto pos = m_begin + count + 1;
			for (auto it = pos; it != m_end; it++) allocator_traits::destroy(m_alloc, m_begin);
			m_end = pos;
		};
	}
	constexpr void reserve(size_type count) {
		auto cap = capacity();

		if (count > cap) {
			if (count > maxSize()) throw std::length_error("lsd::BasicString::reserve(): Count exceded maximum allocation size");
			else {
				auto s = size();
				auto oldBegin = std::exchange(m_begin, allocator_traits::allocate(m_alloc, count));

				if (oldBegin) {
					for (auto beginIt = m_begin, oldBeginIt = oldBegin; oldBeginIt != m_end; beginIt++, oldBeginIt++)
						allocator_traits::construct(m_alloc, beginIt, std::move(*oldBeginIt));

					allocator_traits::deallocate(m_alloc, oldBegin, cap);
				}

				m_end = m_begin + s;
				m_cap = m_begin + count;
			}
		}
	}
	constexpr void shrinkToFit() {
		auto s = size();
		auto cap = capacity();

		if (s < cap) {
			auto oldBegin = std::exchange(m_begin, allocator_traits::allocate(m_alloc, s));

			if (oldBegin) {
				for (auto beginIt = m_begin, oldBeginIt = oldBegin; oldBeginIt != m_end; beginIt++, oldBeginIt++)
					allocator_traits::construct(m_alloc, beginIt, std::move(*oldBeginIt));

				allocator_traits::deallocate(m_alloc, oldBegin, cap);
			}

			m_end = m_begin + s;
			m_cap = m_end;
		}
	}
	[[deprecated]] constexpr void shrink_to_fit() {
		shrinkToFit();
	}

	constexpr iterator insert(const_iterator position, const_reference value) {
		return insert(position, 1, value);
	}
	constexpr iterator insert(const_iterator position, rvreference value) {
		auto pos = const_cast<pointer>(position.get());

		auto ptr = eraseAndInsertGap(pos, 0, 1);
		allocator_traits::construct(m_alloc, ptr, std::move(value));
		
		return ptr;
	}
	constexpr iterator insert(const_iterator position, size_type count, const_reference value) {
		auto pos = const_cast<pointer>(position.get());

		if (count != 0) {
			auto ptr = eraseAndInsertGap(pos, 0, count);
			for (auto it = ptr; count != 0; count--, it++) allocator_traits::construct(m_alloc, it, value);

			return ptr;
		} else return pos;
	}
	template <class It> constexpr iterator insert(const_iterator position, It first, It last) requires isIteratorValue<It> {
		auto pos = const_cast<pointer>(position.get());
		
		if (first != last) {
			auto ptr = eraseAndInsertGap(pos, 0, last - first);
			for (auto it = ptr; first != last; first++, it++) allocator_traits::construct(m_alloc, it, *first);

			return ptr;
		} else return pos;
	}
	constexpr iterator insert(const_iterator position, init_list ilist) {
		return insert(position, ilist.begin(), ilist.end());
	}

	template <class... Args> constexpr iterator emplace(const_iterator position, Args&&... args) {
		auto pos = const_cast<pointer>(position.get());
		
		auto ptr = eraseAndInsertGap(pos, 0, 1);
		allocator_traits::construct(m_alloc, ptr, std::forward<Args>(args)...);

		return ptr;
	}
	template <class... Args> constexpr reference emplaceBack(Args&&... args) {
		smartReserve(size() + 1);
		allocator_traits::construct(m_alloc, m_end, std::forward<Args>(args)...);
		return *m_end++;
	}
	template <class... Args> [[deprecated]] constexpr reference emplace_back(Args&&... args) {
		return emplaceBack(std::forward<Args>(args)...);
	}

	constexpr void pushBack(const_reference value) {
		append(1, value);
	}
	constexpr void pushBack(rvreference value) {
		smartReserve(size() + 1);
		allocator_traits::construct(m_alloc, m_end++, std::move(value));
	}
	[[deprecated]] constexpr void push_back(const_reference value) {
		pushBack(value);
	}
	[[deprecated]] constexpr void push_back(rvreference value) {
		pushBack(std::move(value));
	}

	constexpr iterator erase(const_iterator pos) {
		assert((pos < end()) && "lsd::Vector::erase: past-end iterator passed to erase!");

		auto it = m_begin + (pos - m_begin);

		allocator_traits::destroy(m_alloc, it);
		allocator_traits::construct(m_alloc, it, std::move(*(it + 1))); // Weird bit here because it needs to reconstruct the previously destroyed piece of memory

		std::move(it + 2, m_end, it + 1); // Move the other elements normally

		popBack();

		return it;
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		auto it = m_begin + (first - m_begin);

		if (first != last) return eraseAndInsertGap(it, last - first, 0);
		else return it;
	}

	constexpr void popBack() {
		allocator_traits::destroy(m_alloc, --m_end);
	}
	[[deprecated]] constexpr void pop_back() {
		popBack();
	}

	constexpr void clear() {
		for (auto it = m_begin; it != m_end; it++) allocator_traits::destroy(m_alloc, m_begin);
		m_end = m_begin;
	}

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_end - m_begin;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return std::min<size_type>(-1, allocator_traits::max_size(m_alloc));
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr size_type capacity() const noexcept {
		return m_cap - m_begin;
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_begin == m_end;
	}

	[[nodiscard]] constexpr const_pointer data() const noexcept {
		return m_begin;
	}
	[[nodiscard]] constexpr pointer data() noexcept {
		return m_begin;
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_alloc;
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return allocator();
	}

	[[nodiscard]] constexpr const_reference at(size_type index) const {
		auto ptr = m_begin + index;
		if (ptr >= m_end) throw std::out_of_range("lsd::Vector::at(): Index exceded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr reference at(size_type index) {
		auto ptr = m_begin + index;
		if (ptr >= m_end) throw std::out_of_range("lsd::Vector::at(): Index exceded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const {
		auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::Vector::operator[]: Index exceded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr reference operator[](size_type index) {
		auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::Vector::operator[]: Index exceded array bounds!");
		return *ptr;
	}

private:
	[[no_unique_address]] allocator_type m_alloc { };

	pointer m_begin { };
	pointer m_end { };
	pointer m_cap { };

	constexpr void smartReserve(size_type size) {
		auto cap = capacity();

		if (size > cap) {
			auto newCap = cap * 2;
			reserve((newCap < size) ? size : newCap);
		}
	}
	
	template <class It> constexpr void moveAssign(It first, It last, const_alloc_reference alloc) requires isIteratorValue<It> {
		m_alloc = alloc;

		if (first != last) {
			auto count = last - first;
			smartReserve(count);

			for (; first != last; first++, m_end++) allocator_traits::construct(m_alloc, m_end, std::move(*first));
		}
	}
	constexpr void append(size_type count, const_reference value) noexcept {
		smartReserve(size() + count);
		for (; count > 0; count--, m_end++) allocator_traits::construct(m_alloc, m_end, value);
	}

	constexpr pointer eraseAndInsertGap(pointer position, size_type eraseCount, size_type gapSize) { // Does not check for validity of eraseCount or gapSize
		auto oldSize = size();
		auto oldCap = capacity();

		auto newSize = oldSize + gapSize - eraseCount;

		if (newSize > oldCap) {
			// Convert position to index
			auto index = position - m_begin;

			// Reserve memory without constructing new memory, similar to smartReserve()
			auto doubleCap = oldCap * 2;
			auto reserveCount = (newSize > doubleCap) ?newSize : doubleCap;
			auto oldBegin = std::exchange(m_begin, allocator_traits::allocate(m_alloc, reserveCount));
			
			// Calculate new pointers
			m_end = m_begin + newSize;
			m_cap = m_begin + reserveCount;

			if (oldBegin) {
				// Prepare some iterators for the following parts
				auto it = m_begin;
				auto begIt = oldBegin;

				// Re-construct the vector in front of pos/position
				for (; it < (m_begin + index); it++, begIt++)
					allocator_traits::construct(m_alloc, it, *begIt);
				
				it += gapSize;
				begIt += eraseCount;

				// Reconstruct the remaining parts of the vector
				for (; it <= m_end; it++, begIt++)
					allocator_traits::construct(m_alloc, it, *begIt);

				allocator_traits::deallocate(m_alloc, oldBegin, oldCap);
			}

			return m_begin + index;
		} else {
			auto oldEnd = std::exchange(m_end, m_end + gapSize - eraseCount);	

			if (oldSize != 0) {
				auto endIt = m_end;
				auto oldEndIt = oldEnd;
				auto moveBegin = position + std::min(eraseCount, gapSize);

				for (auto count = m_end - oldEnd; count > 0 && oldEndIt > moveBegin; --oldEndIt, --endIt, count--) {
					allocator_traits::construct(m_alloc, endIt, std::move(*oldEndIt));
					allocator_traits::destroy(m_alloc, oldEndIt);
				}

				std::move_backward(moveBegin, oldEndIt, endIt);

				for (; oldEndIt != moveBegin; --endIt, --oldEndIt) { // Custom std::move_backward
					*endIt = std::move(*oldEndIt);
					allocator_traits::destroy(m_alloc, oldEndIt);
				}
			}
			
			return position;
		}
	}
};

} // namespace lsd
