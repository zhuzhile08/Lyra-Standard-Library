/*************************
 * @file Vector.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Vector implementation
 * 
 * @date 2024-02-24
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Iterators.h"

#include <cstdlib>
#include <cassert>
#include <utility>
#include <functional>
#include <memory>
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
	using alloc_traits = std::allocator_traits<allocator_type>;

	using value_type = Ty;
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

	using container = Vector;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	static constexpr double resizeFactor = 2.0f;

private:
	static constexpr size_type resizeDivFactor = static_cast<size_type>(1 / (resizeFactor - 1.0f));

public:

	constexpr Vector() noexcept(noexcept(allocator_type())) : Vector(allocator_type()) { }
	constexpr explicit Vector(const_alloc_reference alloc) noexcept(noexcept(allocator_type())) : m_alloc(alloc) { }

	constexpr Vector(size_type count, const_reference value, const_alloc_reference alloc = allocator_type()) : m_alloc(alloc) {
		if (count != 0) {
			allocBuffer(count);
			pushBackValues(count, value);
		}
	}
	constexpr explicit Vector(size_type count, const_alloc_reference alloc = allocator_type()) : 
		Vector(count, value_type { }, alloc) { }

	template <std::random_access_iterator Iter> constexpr Vector(Iter first, Iter last, const_alloc_reference alloc = allocator_type()) : m_alloc(alloc) {
		if (first != last) {
			allocBuffer(std::distance(first, last));
			moveFromRange(first, last);
		}
	}
	template <std::input_iterator Iter> constexpr Vector(Iter first, Iter last, const_alloc_reference alloc = allocator_type())
		requires(!std::random_access_iterator<Iter>) : m_alloc(alloc) {

		for (; first != last; first++)
			emplaceBack(std::move(*first));
	}

	constexpr Vector(const_container_reference other, const std::type_identity_t<allocator_type>& alloc) : m_alloc(alloc) {
		if (auto siz = other.size(); siz != 0) {
			allocBuffer(siz);
			copyFromRange(other.m_begin, other.m_end);
		}
	}
	constexpr Vector(const_container_reference other) :
		Vector(other, alloc_traits::select_on_container_copy_construction(other.allocator())) { }

	constexpr Vector(container_rvreference other) noexcept :
		m_alloc(std::move(other.m_alloc)),
		m_begin(std::exchange(other.m_begin, pointer { })),
		m_end(std::exchange(other.m_end, pointer { })),
		m_cap(std::exchange(other.m_cap, pointer { })) { }
	constexpr Vector(container_rvreference other, const std::type_identity_t<allocator_type>& alloc)
		requires(alloc_traits::is_always_equal::value) :
		m_alloc(alloc),
		m_begin(std::exchange(other.m_begin, pointer { })),
		m_end(std::exchange(other.m_end, pointer { })),
		m_cap(std::exchange(other.m_cap, pointer { })) { }
	constexpr Vector(container_rvreference other, const std::type_identity_t<allocator_type>& alloc)
		requires(!alloc_traits::is_always_equal::value) : m_alloc(alloc) {

		if (m_alloc == other.m_alloc) {
			swapBuffers(other);
		} else {
			allocBuffer(other.size());
			moveFromRange(other.m_begin, other.m_end);
		}
	}

	constexpr Vector(init_list ilist, const_alloc_reference alloc = allocator_type()) : Vector(ilist.begin(), ilist.end(), alloc) { }

	constexpr ~Vector() {
		if (m_begin) {
			for (auto it = m_begin; it != m_end; it++)
				alloc_traits::destroy(m_alloc, it);
			alloc_traits::deallocate(m_alloc, m_begin, m_cap - m_begin);
			
			m_begin = nullptr;
			m_end = nullptr;
			m_cap = nullptr;
		}
	}


	constexpr container_reference operator=(const_container_reference other) {
		if (this == &other)
			return *this;

		if constexpr (
			alloc_traits::propagate_on_container_copy_assignment::value &&
			!alloc_traits::is_always_equal::value
		) {
			if (m_begin) {
				clear();

				if (m_alloc == other.m_alloc)
					reserve(other.size());
				else
					allocBuffer(other.size(), std::exchange(m_alloc, other.m_alloc));
			} else {
				if (m_alloc != other.m_alloc)
					m_alloc = other.m_alloc;

				allocBuffer(other.size());
			}
		} else {
			if (m_begin)
				clear();
			
			reserve(other.size());
		}

		copyFromRange(other.m_begin, other.m_end);

		return *this;
	}
	constexpr container_reference operator=(container_rvreference other)
		noexcept(alloc_traits::propagate_on_container_move_assignment::value || alloc_traits::is_always_equal::value) {

		if (this == &other)
			return *this;

		if constexpr (
			!alloc_traits::propagate_on_container_move_assignment::value &&
			!alloc_traits::is_always_equal::value
		) { // If the allocator shouldn't be propagated but the allocators aren't always equal, rebuild the entire vector
			if (m_alloc != other.m_alloc) {
				if (m_begin)
					clear();

				reserve(other.size());
				moveFromRange(other.m_begin, other.m_end);

				return *this;
			}
		} else if constexpr (alloc_traits::propagate_on_container_move_assignment::value)
			other.m_alloc = std::move(std::exchange(m_alloc, other.m_alloc));

		swapBuffers(other);

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
	template <std::random_access_iterator Iter> constexpr void assign(Iter first, Iter last) {
		clear();
		if (first == last)
			return;
	
		reserve(std::distance(first, last));
		moveFromRange(first, last);
	}
	template <std::input_iterator Iter> constexpr void assign(Iter first, Iter last) requires(!std::random_access_iterator<Iter>) {
		clear();
		for (; first != last; first++)
			emplaceBack(std::move(*first));
	}
	constexpr void assign(init_list ilist) {
		assign(ilist.begin(), ilist.end());
	}

	constexpr void swap(container_reference other)
		noexcept(alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value) {

		if constexpr (
			!alloc_traits::propagate_on_container_swap::value &&
			!alloc_traits::is_always_equal::value
		) {
			const auto siz = size();
			const auto oSiz = other.size();

			if (siz < oSiz)
				swapContents(other, oSiz);
			else if (oSiz < siz)
				other.swapContents(*this, siz);
			else
				std::swap_ranges(m_begin, m_end, other.m_begin);
		} else if constexpr (alloc_traits::propagate_on_container_swap::value &&
							 !alloc_traits::is_always_equal::value)
			if (m_alloc != other.m_alloc)
				std::swap(m_alloc, other.m_alloc);

		swapBuffers(other);
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


	constexpr void resize(size_type count, const_reference value) {
		auto siz = size();
		if (count > siz) {
			smartReserve(count);
			pushBackValues(count - siz, value);
		} else if (count < siz)
			destroyEnd(siz - count);
	}
	constexpr void resize(size_type count) {
		resize(count, value_type { });
	}
	constexpr void reserve(size_type count) {
		auto cap = capacity();

		if (count > cap)
			reserveImpl(count);
	}
	constexpr void shrinkToFit() {
		auto siz = size();
		auto cap = capacity();

		if (siz != 0 && siz < cap)
			reserveImpl(siz);
	}
	[[deprecated]] constexpr void shrink_to_fit() {
		shrinkToFit();
	}


	constexpr iterator insert(const_iterator position, const_reference value) {
		return insertGap(const_cast<pointer>(position.get()), 1, &Vector::pushBackValues, &Vector::pushBackValuesAssign, 1, value);
	}
	constexpr iterator insert(const_iterator position, rvreference value) {
		return insertGap(const_cast<pointer>(position.get()), 1, &Vector::emplaceValue<value_type>, &Vector::emplaceValueAssign<value_type>, std::move(value));
	}
	constexpr iterator insert(const_iterator position, size_type count, const_reference value) {
		if (count != 0)
			return insertGap(const_cast<pointer>(position.get()), count, &Vector::pushBackValues, &Vector::pushBackValuesAssign, count, value);
		else
			return const_cast<pointer>(position.get());
	}
	template <std::input_iterator Iter> constexpr iterator insert(const_iterator position, Iter first, Iter last) {
		if (const auto dist = std::distance(first, last); dist != 0)
			return insertGap(const_cast<pointer>(position.get()), dist, &Vector::moveFromRange<Iter>, &Vector::moveFromRangeAssign<Iter>, first, last);
		else
			return const_cast<pointer>(position.get());
	}
	constexpr iterator insert(const_iterator position, init_list ilist) {
		return insert(position, ilist.begin(), ilist.end());
	}

	template <class... Args> constexpr iterator emplace(const_iterator position, Args&&... args) {
		return insertGap(const_cast<pointer>(position.get()), 1, &Vector::emplaceValue<Args...>, &Vector::emplaceValueAssign<Args...>, std::forward<Args>(args)...);
	}
	template <class... Args> constexpr reference emplaceBack(Args&&... args) {
		smartReserve(size() + 1);
		alloc_traits::construct(m_alloc, m_end, std::forward<Args>(args)...);
		return *m_end++;
	}
	template <class... Args> [[deprecated]] constexpr reference emplace_back(Args&&... args) {
		return emplaceBack(std::forward<Args>(args)...);
	}

	constexpr void pushBack(const_reference value) {
		smartReserve(size() + 1);
		alloc_traits::construct(m_alloc, m_end++, value);
	}
	constexpr void pushBack(rvreference value) {
		smartReserve(size() + 1);
		alloc_traits::construct(m_alloc, m_end++, std::move(value));
	}
	[[deprecated]] constexpr void push_back(const_reference value) {
		pushBack(value);
	}
	[[deprecated]] constexpr void push_back(rvreference value) {
		pushBack(std::move(value));
	}


	constexpr iterator erase(const_iterator pos) {
		assert((pos < m_end && pos >= m_begin) && "lsd::Vector::erase: Past-end iterator passed to erase!");

		auto it = const_cast<pointer>(pos.get());

		if (auto next = it + 1; next < m_end) // The element to be erased is not the last one
			std::move(next, m_end, it); // Move the other elements normally

		alloc_traits::destroy(m_alloc, --m_end);

		return it;
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		assert((first < m_end && first >= m_begin && last <= m_end && last >= m_begin && last >= first) && "lsd::Vector::erase: Invalid range passed to erase!");

		auto it = const_cast<pointer>(first.get());

		if (last == first)
			return it;

		if (last != m_end)
			std::move(const_cast<pointer>(last.get()), m_end, it);

		destroyEnd(last - first);

		return it;
	}

	constexpr void popBack() {
		alloc_traits::destroy(m_alloc, --m_end);
	}
	[[deprecated]] constexpr void pop_back() {
		popBack();
	}

	constexpr void clear() {
		while (m_end != m_begin)
			alloc_traits::destroy(m_alloc, --m_end);
	}


	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_begin ? m_end - m_begin : 0;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return alloc_traits::max_size(m_alloc);
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr size_type capacity() const noexcept {
		return m_begin ? m_cap - m_begin : 0;
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
		if (ptr >= m_end) throw std::out_of_range("lsd::Vector::at(): Index exceeded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr reference at(size_type index) {
		auto ptr = m_begin + index;
		if (ptr >= m_end) throw std::out_of_range("lsd::Vector::at(): Index exceeded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr const_reference operator[](size_type index) const {
		auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::Vector::operator[]: Index exceeded array bounds!");
		return *ptr;
	}
	[[nodiscard]] constexpr reference operator[](size_type index) {
		auto ptr = m_begin + index;
		assert((ptr < m_end) && "lsd::Vector::operator[]: Index exceeded array bounds!");
		return *ptr;
	}


	friend constexpr bool operator==(const_container_reference lhs, const_container_reference rhs) {
		if (lhs.size() != rhs.size())
			return false;

		for (auto lit = lhs.begin(), rit = rhs.begin(); lit != lhs.end(); ++lit, ++rit)
			if (*lit != *rit)
				return false;

		return true;
	}

	friend constexpr auto operator<=>(const_container_reference lhs, const_container_reference rhs) {
		return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

private:
	[[no_unique_address]] allocator_type m_alloc { };

	pointer m_begin { };
	pointer m_end { };
	pointer m_cap { };

	constexpr void smartReserve(size_type cap) {
		auto oldCap = capacity();

		if (cap > oldCap)
			reserveImpl(std::max(cap, oldCap + oldCap / resizeDivFactor));
	}


	constexpr void allocateFresh(size_type cap) {
		m_begin = alloc_traits::allocate(m_alloc, cap);
		m_end = m_begin;
		m_cap = m_begin + cap;
	}

	constexpr void allocBuffer(size_type cap) {
		if (cap > maxSize())
			throw std::length_error("lsd::Vector::allocBuffer(): Size exceeded maximum allocation size!");

		allocateFresh(cap);
	}
	constexpr void allocBuffer(size_type cap, allocator_type& oldAlloc) {
		if (cap > maxSize())
			throw std::length_error("lsd::Vector::allocBuffer(): Size exceeded maximum allocation size!");

		while (m_end != m_begin)
			alloc_traits::destroy(oldAlloc, --m_end);

		alloc_traits::deallocate(oldAlloc, m_begin, m_cap - m_begin);

		allocateFresh(cap);
	}

	constexpr void reserveImpl(size_type cap) {
		if (cap > maxSize())
			throw std::length_error("lsd::Vector::reserveImpl(): Size exceeded maximum allocation size!");

		if (m_begin) {
			auto oldBegin = std::exchange(m_begin, alloc_traits::allocate(m_alloc, cap));
			auto oldEnd = std::exchange(m_end, m_begin);
			auto oldCap = std::exchange(m_cap, m_begin + cap);

			moveFromRange(oldBegin, oldEnd);

			alloc_traits::deallocate(m_alloc, oldBegin, oldCap - oldBegin);
		} else
			allocateFresh(cap);
	}


	template <std::input_iterator Iter> constexpr void copyFromRange(Iter first, Iter last) {
		for (; first != last; first++, m_end++)
			alloc_traits::construct(m_alloc, m_end, *first);
	}

	template <std::input_iterator Iter> constexpr void moveFromRange(Iter first, Iter last) {
		for (; first != last; first++, m_end++)
			alloc_traits::construct(m_alloc, m_end, std::move(*first));
	}
	template <std::input_iterator Iter> constexpr void moveFromRangeAssign(Iter first, Iter last) {
		for (; first != last; first++, m_end++)
			*m_end = std::move(*first);
	}

	constexpr void pushBackValues(size_type count, const_reference value) {
		for (; count > 0; count--, m_end++)
			alloc_traits::construct(m_alloc, m_end, value);
	}
	constexpr void pushBackValuesAssign(size_type count, const_reference value) {
		for (; count > 0; count--, m_end++)
			*m_end = value;
	}

	template <class... Args> constexpr void emplaceValue(Args&&... args) {
		alloc_traits::construct(m_alloc, m_end++, std::forward<Args>(args)...);
	}
	template <class... Args> constexpr void emplaceValueAssign(Args&&... args) {
		*m_end++ = value_type(std::forward<Args>(args)...);
	}

	constexpr void destroyEnd(size_type count) {
		for (--m_end; count > 0; m_end--, count--)
			alloc_traits::destroy(m_alloc, m_end);

		++m_end;
	}

	constexpr void swapContents(container_reference other, size_type otherSize) {
		reserve(otherSize);
		const auto otherEnd = std::swap_ranges(m_begin, m_end, other.m_begin);
		
		for (auto it = otherEnd; it != other.m_end; it++, m_end++) {
			alloc_traits::construct(m_alloc, m_end, std::move(*it));
			alloc_traits::destroy(other.m_alloc, it);
		}

		other.m_end = otherEnd;
	}


	constexpr void swapBuffers(container_reference other) noexcept {
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
		std::swap(m_cap, other.m_cap);
	}

	template <class Inserter1, class Inserter2, class... Args>
	constexpr pointer insertGap(pointer position, 
								size_type gapSize,
							Inserter1&& inserter1,
								Inserter2&& inserter2,
								Args&&... args) {
		const auto oldSize = size();
		const auto oldCap = capacity();

		const auto newSize = oldSize + gapSize;

		if (newSize > oldCap) { // New memory needs to be reserved
			const auto offset = position - m_begin;
			const auto oldBegin = m_begin;
			const auto oldEnd = m_end;

			allocateFresh(std::max(newSize, oldCap + oldCap * resizeDivFactor));

			if (oldBegin != oldEnd) {
				moveFromRange(oldBegin, position);
				std::invoke(std::forward<Inserter1>(inserter1), this, std::forward<Args>(args)...);
				moveFromRange(position, oldEnd);

				for (auto it = oldEnd; it != oldBegin;) // Move backwards to destroy everything, oldEnd is currently in cache
					alloc_traits::destroy(m_alloc, --it);

				alloc_traits::deallocate(m_alloc, oldBegin, oldCap);
			}

			return m_begin + offset;
		} else {
			auto oldEnd = m_end;
			m_end += gapSize;

			if (oldSize != 0) {
				const auto oldEnd = m_end;
				const auto newEnd = m_end + gapSize;

				auto endIt = newEnd;

				while (endIt != oldEnd && m_end != position) // First, the back elements are constructed newly
					alloc_traits::construct(m_alloc, --endIt, std::move(*--m_end));

				std::move_backward(position, m_end, endIt); // As the rest is already constructed, they can be moved into the proper position

				m_end = position;
				std::invoke(std::forward<Inserter2>(inserter2), this, std::forward<Args>(args)...);

				m_end = newEnd;
			}

			return position;
		}
	}
};

} // namespace lsd
