/*************************
 * @file Iterators.h
 * @author zhuzhile08 (zhuzhile08@gmail.com)
 * 
 * @brief a iterator base class
 * 
 * @date 2023-03-06
 * 
 * @copyright Copyright (c) 2023
 *************************/

#pragma once

#include "ForwardListNode.h"

#include <cstddef>
#include <type_traits>
#include <iterator>

namespace lsd {

template <class Ty> class Iterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::contiguous_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using const_pointer = const pointer;
	using pointer_const = const_value*;
	using reference = value_type&;
	using const_reference = const_value&;
	using container = Iterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr Iterator() noexcept = default;
	constexpr Iterator(pointer pointer) noexcept : m_pointer(pointer) { }
	constexpr Iterator(reference reference) noexcept : m_pointer(&reference) { }

	constexpr reference operator*() { return *m_pointer; }
	constexpr const_reference operator*() const { return *m_pointer; }
	constexpr pointer operator->() noexcept { return m_pointer; }
	constexpr const_pointer operator->() const noexcept { return m_pointer; }
	constexpr pointer get() noexcept { return m_pointer; }
	constexpr const_pointer get() const noexcept { return m_pointer; }

	constexpr container_reference operator++() noexcept { 
		++m_pointer; 
		return *this; 
	}
	constexpr container operator++(int) noexcept { 
		container tmp = *this; 
		++(*this); 
		return tmp; 
	}
	constexpr container_reference operator--() noexcept { 
		--m_pointer; 
		return *this; 
	}
	constexpr container operator--(int) noexcept { 
		container tmp = *this; 
		--(*this); 
		return tmp; 
	}

	friend constexpr container operator+(const_container_reference it, std::size_t i) noexcept {
		return container(it.m_pointer + i);
	}
	friend constexpr container operator+(std::size_t i, const_container_reference it) noexcept {
		return container(it.m_pointer + i);
	}
	friend constexpr container operator-(const_container_reference it, std::size_t i) noexcept {
		return it + (-i); 
	}
	friend constexpr container operator-(std::size_t i, const_container_reference it) noexcept {
		return it + (-i); 
	}
	friend constexpr std::size_t operator-(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer - second.m_pointer;
	}
	friend constexpr std::size_t operator-(pointer_const first, const_container_reference second) noexcept {
		return first - second.m_pointer;
	}
	friend constexpr std::size_t operator-(const_container_reference first, pointer_const second) noexcept {
		return first.m_pointer - second;
	}

	friend container& operator+=(container_reference it, std::size_t i) noexcept {
		it = it + i;
		return it;
	}
	friend container& operator-=(container_reference it, std::size_t i) noexcept {
		it = it - i;
		return it;
	}

	constexpr operator Iterator<const_value>() const noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator const_pointer() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }
	friend constexpr bool operator!=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer != second.m_pointer; }
	friend constexpr bool operator>(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer > second.m_pointer; }
	friend constexpr bool operator<(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer < second.m_pointer; }
	friend constexpr bool operator>=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer >= second.m_pointer; }
	friend constexpr bool operator<=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer <= second.m_pointer; }

private:
	pointer m_pointer;
};

template <class Ty> class ReverseIterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::contiguous_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using const_pointer = const pointer;
	using pointer_const = const_value*;
	using reference = value_type&;
	using const_reference = const_value&;
	using container = ReverseIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr ReverseIterator() noexcept = default;
	constexpr ReverseIterator(pointer pointer) noexcept : m_pointer(pointer) { }
	constexpr ReverseIterator(reference reference) noexcept : m_pointer(&reference) { }

	constexpr reference operator*() { return *m_pointer; }
	constexpr const_reference operator*() const { return *m_pointer; }
	constexpr pointer operator->() noexcept { return m_pointer; }
	constexpr const_pointer operator->() const noexcept { return m_pointer; }
	constexpr pointer get() noexcept { return m_pointer; }
	constexpr const_pointer get() const noexcept { return m_pointer; }

	constexpr container_reference operator++() noexcept { 
		--m_pointer; 
		return *this; 
	}
	constexpr container operator++(int) noexcept { 
		container tmp = *this; 
		++(*this); 
		return tmp; 
	}
	constexpr container_reference operator--() noexcept { 
		++m_pointer; 
		return *this; 
	}
	constexpr container operator--(int) noexcept { 
		container tmp = *this; 
		--(*this); 
		return tmp; 
	}

	friend constexpr container operator+(const_container_reference it, std::size_t i) noexcept {
		return container(it.m_pointer - i);
	}
	friend constexpr container operator+(std::size_t i, const_container_reference it) noexcept {
		return container(it.m_pointer - i);
	}
	friend constexpr container operator-(const_container_reference it, std::size_t i) noexcept {
		return it + i; 
	}
	friend constexpr container operator-(std::size_t i, const_container_reference it) noexcept {
		return it + i; 
	}
	friend constexpr std::size_t operator-(const_container_reference first, const_container_reference second) noexcept {
		return second.m_pointer - first.m_pointer;
	}
	friend constexpr std::size_t operator-(pointer_const first, const_container_reference second) noexcept {
		return second.m_pointer - first;
	}
	friend constexpr std::size_t operator-(const_container_reference first, pointer_const second) noexcept {
		return second - first.m_pointer;
	}

	friend container& operator+=(container_reference it, std::size_t i) noexcept {
		it = it + i;
		return it;
	}
	friend container& operator-=(container_reference it, std::size_t i) noexcept {
		it = it - i;
		return it;
	}

	constexpr operator Iterator<const_value>() const noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator const_pointer() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }
	friend constexpr bool operator!=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer != second.m_pointer; }
	friend constexpr bool operator>(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer < second.m_pointer; }
	friend constexpr bool operator<(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer > second.m_pointer; }
	friend constexpr bool operator>=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer <= second.m_pointer; }
	friend constexpr bool operator<=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer >= second.m_pointer; }

private:
	pointer m_pointer;
};

template <class Ty> class ForwardListIterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using const_pointer = const pointer;
	using reference = value_type&;
	using const_reference = const_value&;

	using node_base = detail::ForwardListNodeBase;
	using node_type = detail::ForwardListNode<value_type>;
	using node_pointer = node_type*;

	using container = ForwardListIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr ForwardListIterator() noexcept = default;
	constexpr ForwardListIterator(node_base* pointer) noexcept : m_pointer(pointer) { }

	constexpr reference operator*() { return static_cast<node_pointer>(m_pointer)->value; }
	constexpr const_reference operator*() const { return static_cast<const node_type*>(m_pointer)->value; }
	constexpr pointer operator->() noexcept { return &static_cast<node_pointer>(m_pointer)->value; }
	constexpr const_pointer operator->() const noexcept { return &static_cast<const node_type*>(m_pointer)->value; }
	constexpr node_base* get() noexcept { return m_pointer; }
	constexpr const node_base* get() const noexcept { return m_pointer; }

	constexpr container_reference operator++() noexcept { 
		m_pointer = m_pointer->next; 
		return *this; 
	}
	constexpr container operator++(int) noexcept { 
		container tmp = *this; 
		++(*this); 
		return tmp; 
	}

	constexpr operator ForwardListIterator<const_value>() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }
	friend constexpr bool operator!=(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer != second.m_pointer; }

private:
	node_base* m_pointer;
};

template <class, class = void> struct IsIterator : public std::false_type { };
template <class Ty> struct IsIterator<Ty, std::void_t<
	typename std::iterator_traits<Ty>::difference_type,
	typename std::iterator_traits<Ty>::pointer,
	typename std::iterator_traits<Ty>::reference,
	typename std::iterator_traits<Ty>::value_type,
	typename std::iterator_traits<Ty>::iterator_category
>> : public std::true_type { };

template <class Ty> inline constexpr bool isIteratorValue = IsIterator<Ty>::value;

}
