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


#include <cstddef>
#include <concepts>
#include <iterator>


namespace lsd {

template <class Ty> class ContinuousIterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::contiguous_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using pointer_const = const_value*;
	using reference = value_type&;

	using container = ContinuousIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr ContinuousIterator() noexcept = default;
	constexpr ContinuousIterator(pointer pointer) noexcept : m_pointer(pointer) { }
	explicit constexpr ContinuousIterator(reference reference) noexcept : m_pointer(&reference) { }


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

	constexpr operator ContinuousIterator<const_value>() const noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer_const() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer == second.m_pointer;
	}
	friend constexpr auto operator<=>(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer <=> second.m_pointer;
	}

private:
	pointer m_pointer;
};

template <class Ty> class ReverseContinuousIterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::contiguous_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using pointer_const = const_value*;
	using reference = value_type&;

	using container = ReverseContinuousIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	using iterator_type = ContinuousIterator<value_type>;


	constexpr ReverseContinuousIterator() noexcept = default;
	constexpr ReverseContinuousIterator(pointer pointer) noexcept : m_pointer(pointer) { }
	explicit constexpr ReverseContinuousIterator(iterator_type iter) : m_pointer(iter.m_pointer) { }
	explicit constexpr ReverseContinuousIterator(reference reference) noexcept : m_pointer(&reference) { }


	constexpr reference operator*() const { return *m_pointer; }
	constexpr pointer operator->() const noexcept { return m_pointer; }
	constexpr pointer get() const noexcept { return m_pointer; }
	constexpr iterator_type base() const noexcept { return m_pointer ? m_pointer + 1 : nullptr; }

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
		return container(it.m_pointer + i); 
	}
	friend constexpr container operator-(std::size_t i, const_container_reference it) noexcept {
		return container(it.m_pointer + i); 
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
		it = it - i;
		return it;
	}
	friend container& operator-=(container_reference it, std::size_t i) noexcept {
		it = it + i;
		return it;
	}

	constexpr operator ReverseContinuousIterator<const_value>() const noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer_const() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer == second.m_pointer;
	}
	friend constexpr auto operator<=>(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer <=> second.m_pointer;
	}

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


template <class Ty> concept ReverseIterator =
	std::input_or_output_iterator<Ty> &&
	requires(Ty iter) {
		typename Ty::iterator_type;
		{ iter.base() } -> std::convertible_to<typename Ty::iterator_type>;
	};

} // namespace lsd
