/*************************
 * @file Iterators.h
 * @author zhuzhile08 (zhuzhile08@gmail.com)
 * 
 * @brief a iterator base class
 * 
 * @date 2023-03-06
 * @copyright Copyright (c) 2023
 *************************/

#pragma once

#include "Detail/ForwardListNode.h"

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
	using pointer_const = const_value*;
	using reference = value_type&;

	using container = Iterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr Iterator() noexcept = default;
	constexpr Iterator(pointer pointer) noexcept : m_pointer(pointer) { }
	explicit constexpr Iterator(reference reference) noexcept : m_pointer(&reference) { }


	constexpr reference operator*() const { return *m_pointer; }
	constexpr pointer operator->() const noexcept { return m_pointer; }
	constexpr pointer get() const noexcept { return m_pointer; }

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
	constexpr explicit operator pointer_const() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }
	friend constexpr auto operator<=>(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer <=> second.m_pointer; }

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
	using pointer_const = const_value*;
	using reference = value_type&;

	using container = ReverseIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr ReverseIterator() noexcept = default;
	constexpr ReverseIterator(pointer pointer) noexcept : m_pointer(pointer) { }
	explicit constexpr ReverseIterator(reference reference) noexcept : m_pointer(&reference) { }


	constexpr reference operator*() const { return *m_pointer; }
	constexpr pointer operator->() const noexcept { return m_pointer; }
	constexpr pointer get() const noexcept { return m_pointer; }

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

	constexpr operator ReverseIterator<const_value>() const noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer_const() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }
	friend constexpr auto operator<=>(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer <=> second.m_pointer; }

private:
	pointer m_pointer;
};


template <class Ty> concept AppendableContainer = requires(Ty c, Ty::value_type v) {
	typename Ty::value_type;
	c.push_back(v);
	c.push_back(std::move(v));
};

template <AppendableContainer Ty> class BackInsertIterator {
public:
	using iterator_category = std::output_iterator_tag;
	using value_type = void;
	using difference_type = std::ptrdiff_t;
	using pointer = void;
	using reference = void;
	using container_type = Ty;

	constexpr explicit BackInsertIterator(container_type& c) noexcept : m_container(&c) { }

	constexpr BackInsertIterator& operator=(const typename container_type::value_type& value) {
		m_container->push_back(value);
		return *this;
	}
	constexpr BackInsertIterator& operator=(typename container_type::value_type&& value) {
		m_container->push_back(std::move(value));
		return *this;
	}

	constexpr BackInsertIterator& operator*() { return *this; }
	constexpr BackInsertIterator& operator++() { return *this; }
	constexpr BackInsertIterator& operator++(int) { return *this; }

private:
	container_type* m_container;
};


// Iterator trait

template <class, class = void> struct IsIterator : public std::false_type { };
template <class Ty> struct IsIterator<Ty, std::void_t<
	typename std::iterator_traits<Ty>::difference_type,
	typename std::iterator_traits<Ty>::pointer,
	typename std::iterator_traits<Ty>::reference,
	typename std::iterator_traits<Ty>::value_type,
	typename std::iterator_traits<Ty>::iterator_category
>> : public std::true_type { };

template <class Ty> inline constexpr bool isIteratorValue = IsIterator<Ty>::value;


// Iterator concepts

template <class Ty> concept IteratorType = isIteratorValue<Ty>;

template <class Ty> concept ForwardContinuousIteratorType = requires(Ty it, std::size_t n) {
	isIteratorValue<Ty>;
	it + n;
};

template <class Ty> concept BackwardsContinuousIteratorType = requires(Ty it, std::size_t n) {
	isIteratorValue<Ty>;
	it - n;
};

template <class Ty> concept ContinuousIteratorType = requires(Ty first, Ty second, std::size_t n) {
	isIteratorValue<Ty>;
	first - second;
	first + n;
	first - n;
};

}
