/************************
 * @file BasicBasicLongString.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Memory allocated string implementation
 * 
 * @date 2026-06-21
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "StringCore.hpp"

#include <utility>
#include <stdexcept>
#include <algorithm>
#include <memory>


namespace lsd {

namespace string {

template <class CharTy, class Traits, class Alloc, std::size_t SmallStringCapacity> class BasicLongString {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using traits_type = Traits;
	using allocator_type = Alloc;
	using alloc_reference = allocator_type&;
	using const_alloc_reference = const allocator_type&;
	using alloc_traits = std::allocator_traits<allocator_type>;

	using value_type = CharTy;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using pointer = value_type*;
	using const_pointer = const_value*;

	using container = BasicLongString;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	static constexpr size_type npos = strNPos;
	static constexpr double resizeFactor = 2.0f;

private:
	static constexpr size_type resizeDivFactor = static_cast<size_type>(1.0 / (resizeFactor - 1.0));

	using padding_type = std::conditional_t<std::cmp_greater(SmallStringCapacity, 0), void* const, std::in_place_t>;

public:
	constexpr BasicLongString() noexcept { }

	constexpr BasicLongString(container_rvreference other) noexcept :
		m_begin(std::exchange(other.m_begin, pointer { })),
		m_end(std::exchange(other.m_end, pointer { })),
		m_cap(std::exchange(other.m_cap, pointer { })) { }
	constexpr BasicLongString(container_rvreference other, const_alloc_reference otherAlloc, alloc_reference alloc) noexcept
		requires (alloc_traits::is_always_equal::value):
		m_begin(std::exchange(other.m_begin, pointer { })),
		m_end(std::exchange(other.m_end, pointer { })),
		m_cap(std::exchange(other.m_cap, pointer { })) { }
	constexpr BasicLongString(container_rvreference other, const_alloc_reference otherAlloc, alloc_reference alloc)
		requires (!alloc_traits::is_always_equal::value) {

		if (alloc == otherAlloc) {
			m_begin = std::exchange(other.m_begin, pointer { });
			m_end = std::exchange(other.m_end, pointer { });
			m_cap = std::exchange(other.m_cap, pointer { });
		} else {
			allocateBuffer(alloc, other.size());
			copyFromRange(other.size(), other.m_begin, other.m_end);
			createTerminator();
		}
	}

	constexpr container_reference operator=(container_rvreference other) {
		swapBuffers(other);
		return *this;
	}

	constexpr container_reference operator=(std::nullptr_t) {
		m_begin = pointer { };
		m_end = pointer { };
		m_cap = pointer { };

		return *this;
	}


	constexpr void construct(alloc_reference alloc, size_type count, value_type value) {
		if (count != 0) {
			allocBuffer(alloc, count);
			pushBackValues(count, value);
			createTerminator();
		}
	}

	template <std::random_access_iterator Iter> constexpr void construct(alloc_reference alloc, size_type distance, Iter first, Iter last) {
		if (distance != 0) {
			allocBuffer(alloc, distance);
			copyFromRange(distance, first, last);
			createTerminator();
		}
	}
	template <std::input_iterator Iter> constexpr void construct(alloc_reference alloc, Iter first, Iter last)
		requires(!std::random_access_iterator<Iter>)
	{
		for (; first != last; first++)
			append<false>(alloc, *first);

		createTerminator();
	}

	template <bool Terminate = false>
	constexpr void construct(alloc_reference alloc, container_rvreference other, const_alloc_reference otherAlloc, size_type count) {
		if constexpr (!alloc_traits::is_always_equal::value) if (alloc != otherAlloc) {
			allocateBuffer(alloc, other.size());
			if constexpr (Terminate)
				copyFromRange(other.size(), other.m_begin, other.m_begin + std::min(count, other.size()));
			else
				copyFromRange(other.size(), other.m_begin, other.m_end);
			createTerminator();

			return;
		}
	
		m_begin = std::exchange(other.m_begin, pointer { });
		m_end = std::exchange(other.m_end, pointer { });
		m_cap = std::exchange(other.m_cap, pointer { });

		if constexpr (Terminate)
			terminateAt(m_begin + std::min(count, other.size()));
	}


	constexpr void dealloc(alloc_reference alloc) {
		alloc_traits::deallocate(alloc, m_begin, m_cap - m_begin);

		m_begin = nullptr;
		m_end = nullptr;
		m_cap = nullptr;
	}


	constexpr void move(alloc_reference alloc, container_rvreference other, allocator_type&& otherAlloc)
		noexcept(alloc_traits::propagate_on_container_move_assignment::value || alloc_traits::is_always_equal::value)
	{
		if constexpr (
			!alloc_traits::is_always_equal::value &&
			!alloc_traits::propagate_on_container_move_assignment::value
		) { // If the allocator shouldn't be propagated but the allocators aren't always equal, rebuild the entire vector
			if (!m_begin) {
				allocBuffer(alloc, other.size());
				copyFromRange(other.size(), other.m_begin, other.m_end);
				createTerminator();

				return;
			}
		} else if constexpr (alloc_traits::propagate_on_container_move_assignment::value && !alloc_traits::is_always_equal::value)
			if (alloc != otherAlloc)
				otherAlloc = std::exchange(alloc, std::move(otherAlloc));

		swapBuffers(other);
	}


	constexpr void assign(alloc_reference alloc, size_type count, value_type value) {
		if (count != 0)
			return;

		if (m_begin)
			m_end = m_begin;

		resize(alloc, count, value);
	}

	template <std::random_access_iterator Iter> constexpr void assign(alloc_reference alloc, size_type distance, Iter first, Iter last) {
		m_end = m_begin;

		reserveImpl(alloc, distance);
		copyFromRange(distance, first, last);
		createTerminator();
	}
	template <std::input_iterator Iter> constexpr void assign(alloc_reference alloc, Iter first, Iter last)
		requires(!std::random_access_iterator<Iter>)
	{
		if (first == last) {
			terminateAt(m_begin);
			return;
		}

		m_end = m_begin;
		for (; first != last; first++)
			append<false>(alloc, *first);

		createTerminator();
	}

	constexpr void swap(alloc_reference alloc, bool differentAlloc, container_reference other, alloc_reference otherAlloc)
		noexcept(alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
	{
		if constexpr (!alloc_traits::propagate_on_container_swap::value) if (differentAlloc) {
			const auto siz = size();
			const auto oSiz = other.size();

			if (siz < oSiz)
				swapContents(alloc, other, oSiz);
			else if (oSiz < siz)
				other.swapContents(alloc, *this, siz);
			else
				swapContentsFixed(other);
			
			return;
		}

		if constexpr (alloc_traits::propagate_on_container_swap::value) if (differentAlloc)
			std::swap(alloc, otherAlloc);

		swapBuffers(other);
	}


	// Allocator propagation

	template <bool Move>
	constexpr void propagateAlloc(bool active, alloc_reference alloc, std::conditional_t<Move, alloc_reference, const_alloc_reference> otherAlloc) {
		if (active && m_begin)
			dealloc(alloc);

		if constexpr (Move)
			std::swap(otherAlloc, alloc);
		else
			alloc = otherAlloc;
	}

	static constexpr bool propagateAllocatorCopy(const_alloc_reference alloc, const_alloc_reference otherAlloc) {
		if constexpr (
			alloc_traits::propagate_on_container_copy_assignment::value &&
			!alloc_traits::is_always_equal::value
		) if (alloc != otherAlloc)
			return true;

		return false;
	}
	static constexpr bool reallocOnMove(const_alloc_reference alloc, const_alloc_reference otherAlloc) {
		if constexpr (
			!alloc_traits::propagate_on_container_move_assignment::value &&
			!alloc_traits::is_always_equal::value
		) if (alloc != otherAlloc)
			return true;

		return false;
	}

	static constexpr bool propagateAllocatorSwap(const_alloc_reference alloc, alloc_reference otherAlloc) {
		if constexpr (!alloc_traits::is_always_equal::value)
			if (alloc != otherAlloc)
				return true;

		return false;
	}


	// Capacaity and rescaling
 
	constexpr void resize(alloc_reference alloc, size_type count, value_type value) {
		const auto siz = size();

		if (count > siz) {
			smartReserve(alloc, count);
			pushBackValues(count - siz, value);
			createTerminator();
		} else if (count < siz)
			terminateAt(m_begin + count);
	}

	constexpr void reserve(alloc_reference alloc, size_type count) {
		auto cap = capacity();

		if (count > cap) {
			reserveImpl(alloc, count);
			createTerminator();
		}
	}

	constexpr bool shrinkToFit(alloc_reference alloc) {
		auto siz = size();

		if (siz < SmallStringCapacity)
			return false;

		if (siz != capacity()) {
			reserveImpl(alloc, siz);
			createTerminator();
		}

		return true;
	}


	// Container content modification

	template <bool ThrowException = false>
	constexpr pointer insert(alloc_reference alloc, const_pointer pos, size_type count, value_type value) {
		if constexpr (!ThrowException)
			assert((pos <= m_end) && "lsd::BasicString::insert(): Out of range position!");
		else if (pos > m_end)
			throw std::out_of_range("lsd::BasicString::insert(): Out of range position!");

		auto p = const_cast<pointer>(pos);
		return insertGap(alloc, 0, p, p, count, &BasicLongString::pushBackValues, count, value);
	}

	template <bool ThrowException, std::input_iterator Iter>
	constexpr pointer insert(alloc_reference alloc, const_pointer pos, size_type distance, Iter begin, Iter end) {
		if constexpr (!ThrowException)
			assert((pos <= m_end) && "lsd::BasicString::insert(): Out of range position!");
		else if (pos > m_end)
			throw std::out_of_range("lsd::BasicString::insert(): Out of range position!");

		auto p = const_cast<pointer>(pos);
		return insertGap(alloc, 0, p, p, distance, &BasicLongString::copyFromRange<Iter>, distance, begin, end);
	}

	template <bool CreateTerminator = true> constexpr void append(alloc_reference alloc, value_type value) {
		smartReserve(alloc, size() + 1);
		traits_type::assign(*m_end++, value);

		if constexpr (CreateTerminator)
			createTerminator();
	}

	constexpr void append(alloc_reference alloc, size_type count, value_type value) {
		smartReserve(alloc, size() + value);
		pushBackValues(count, value);
		createTerminator();
	}

	template <std::contiguous_iterator Iter> constexpr void append(alloc_reference alloc, size_type distance, Iter begin, Iter end) {
		smartReserve(alloc, size() + distance);
		copyFromRange(distance, begin, end);
		createTerminator();
	}
	template <std::input_iterator Iter> constexpr void append(alloc_reference alloc, Iter begin, Iter end)
		requires(!std::contiguous_iterator<Iter>)
	{
		if (begin == end)
			return;

		for (; begin != end; begin++)
			append<false>(alloc, *begin);

		createTerminator();
	}

	template <bool ThrowException>
	constexpr void replace(alloc_reference alloc, size_type eraseCount, const_pointer first, const_pointer last, size_type count, value_type value) {
		assert((first <= last && last <= m_end) && "lsd::BasicString::replace(): past-end iterator!");

		if constexpr (ThrowException) if (first > m_end)
			throw std::out_of_range("lsd::BasicString::replace(): Out of range position!");
		
		insertGap(alloc, eraseCount, const_cast<pointer>(first), const_cast<pointer>(last), count, &BasicLongString::pushBackValues, count, value);
	}

	template <bool ThrowException, std::input_iterator Iter>
	constexpr void replace(alloc_reference alloc, size_type eraseCount, const_pointer first, const_pointer last, size_type distance, Iter begin, Iter end) {
		assert((first <= last && last <= m_end) && "lsd::BasicString::replace(): past-end and/or invalid range!");

		if constexpr (ThrowException) if (first > m_end)
			throw std::out_of_range("lsd::BasicString::replace(): Out of range position!");

		insertGap(alloc, eraseCount, const_cast<pointer>(first), const_cast<pointer>(last), distance, &BasicLongString::copyFromRange<Iter>, distance, begin, end);
	}


	constexpr pointer erase(const_pointer pos) {
		assert((pos <= m_end) && "lsd::BasicString::erase: past-end iterator!");

		const auto dst = const_cast<pointer>(pos);
		const auto src = dst + 1;

		traits_type::move(dst, src, m_end - src);
		terminateAt(m_end - 1);

		return dst;
	}

	template <bool ThrowException> constexpr pointer erase(const_pointer first, const_pointer last) {
		assert((last <= m_end && first >= m_begin && first <= last) && "lsd::BasicString::erase: past-end and/or invalid range!");

		if constexpr (ThrowException) if (first > m_end)
			throw std::out_of_range("lsd::BasicString::erase: past-end position!");

		const auto dst = const_cast<pointer>(first);
		const auto dstEnd = const_cast<pointer>(last);
		if (first == last)
			return dst;

		const auto endCount = m_end - last;
		traits_type::move(dst, dstEnd, endCount);
		terminateAt(dst + endCount);

		return dst;
	}


	constexpr void terminateAt(pointer newEnd) {
		m_end = newEnd;
		traits_type::assign(*m_end, value_type { });
	}


	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_end - m_begin;
	}
	[[nodiscard]] constexpr size_type maxSize(const_alloc_reference alloc) const noexcept {
		return alloc_traits::max_size(alloc);
	}
	[[nodiscard]] constexpr size_type capacity() const noexcept {
		return m_cap - m_begin;
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_begin == m_end;
	}

private:
	[[no_unique_address]] padding_type m_padding { };

	pointer m_begin { };
	pointer m_end { };
	pointer m_cap { };


	// Buffer allocation
	
	constexpr void allocateFresh(alloc_reference alloc, size_type cap) {
		m_begin = alloc_traits::allocate(alloc, cap);
		m_cap = m_begin + cap;
		m_end = m_begin;
	}

	constexpr void allocBuffer(alloc_reference alloc, size_type cap) {
		if (cap > maxSize(alloc))
			throw std::length_error("lsd::BasicString::allocBuffer(): Size exceeded maximum allocation size!");

		allocateFresh(alloc, cap);
	}
	constexpr void allocBuffer(alloc_reference alloc, size_type cap, alloc_reference oldAlloc) {
		if (cap > maxSize(alloc))
			throw std::length_error("lsd::BasicString::allocBuffer(): Size exceeded maximum allocation size!");

		alloc_traits::deallocate(oldAlloc, m_begin, m_cap - m_begin);
	
		allocateFresh(alloc, cap);
	}

	constexpr void reserveImpl(alloc_reference alloc, size_type cap) {
		if (cap > maxSize(alloc))
			throw std::length_error("lsd::BasicString::reserveImpl(): Size exceeded maximum allocation size!");

		if (m_begin) {
			auto oldBegin = std::exchange(m_begin, alloc_traits::allocate(alloc, cap));
			auto oldEnd = std::exchange(m_end, m_begin);
			auto oldCap = std::exchange(m_cap, m_begin + cap);

			copyFromRange(oldEnd - oldBegin, oldBegin, oldEnd);

			alloc_traits::deallocate(alloc, oldBegin, oldCap - oldBegin);
		} else
			allocBuffer(alloc, cap);
	}

	constexpr void smartReserve(alloc_reference alloc, size_type cap) {
		auto oldCap = capacity();

		if (cap > oldCap)
			reserveImpl(alloc, std::max(cap, oldCap + oldCap / resizeDivFactor));
	}


	// Container content modification

	template <std::input_iterator Iter> constexpr void copyFromRange(std::size_t distance, Iter first, Iter last) {
		m_end = traits_type::copy(m_end, first, distance) + distance;
	}

	constexpr void pushBackValues(size_type count, value_type value) {
		m_end = traits_type::assign(m_end, count, value) + count;
	}

	constexpr void createTerminator() {
		traits_type::assign(*m_end, value_type { });
	}


	// Swapping implementation. These assume that the container calling the swap is the smaller of the two.

	constexpr pointer swapContentsFixed(container_reference other) noexcept {
		auto otherIt = other.m_begin;
		for (auto it = m_begin; it != m_end; it++, otherIt++) {
			value_type tmp { *it };
			traits_type::assign(*it, *otherIt);
			traits_type::assign(*otherIt, tmp);
		}

		return otherIt;
	}

	constexpr void swapContents(alloc_reference alloc, container_reference other, size_type otherSize) {
		reserve(alloc, otherSize);

		const auto otherIt = swapContentsFixed(other);
		copyFromRange(other.m_end - otherIt, otherIt, other.m_end);
		other.terminateAt(otherIt);
	}

	constexpr void swapBuffers(container_reference other) {
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
		std::swap(m_cap, other.m_cap);
	}


	template <class Inserter, class... Args>
	constexpr pointer insertGap(alloc_reference alloc,
								size_type eraseCount,
								pointer first,
								pointer last,
								size_type gapSize,
								Inserter&& inserter,
								Args&&... args) {
		const auto oldSize = size();
		const auto oldCap = capacity();

		const auto newSize = oldSize + gapSize - eraseCount;

		if (newSize > oldCap) { // New memory needs to be reserved
			const auto oldBegin = m_begin;
			const auto oldEnd = m_end;
			const auto firstOffset = first - oldBegin;

			// Reserve an extra amout of memory, similar to smartReserve()
			allocBuffer(alloc, std::max(newSize, oldCap + oldCap / resizeDivFactor));

			if (oldBegin != oldEnd) { // Copy contents from last container if it was valid
				copyFromRange(firstOffset, oldBegin, first);
				std::invoke(std::forward<Inserter>(inserter), this, std::forward<Args>(args)...);
				copyFromRange(oldEnd - last, last, oldEnd);

				alloc_traits::deallocate(alloc, oldBegin, oldCap);
			}

			createTerminator();

			return m_begin + firstOffset;
		} else { // Continue using old memory
			const auto newEnd = m_end + gapSize - eraseCount;

			traits_type::assign(*newEnd, value_type { }); // Null terminator, probably better for cache locality if created here
			traits_type::move(first + gapSize, last, m_end - last);

			m_end = first;
			std::invoke(std::forward<Inserter>(inserter), this, std::forward<Args>(args)...);

			m_end = newEnd;

			return first;
		}
	}


	template <class, class, class> friend class ::lsd::BasicString;
};

} // namespace string

} // namespace lsd
