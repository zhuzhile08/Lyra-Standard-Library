/************************
 * @file Iterator.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Forward list iterator
 * 
 * @date 2026-07-13
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "ForwardListNode.hpp"

#include <cstddef>
#include <iterator>


namespace lsd {

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

	using node_base = forward_list::ForwardListNodeBase;
	using node_type = forward_list::ForwardListNode<value_type>;
	using node_pointer = node_type*;

	using container = ForwardListIterator;
	using container_reference = container&;
	using const_container_reference = const container&;

	constexpr ForwardListIterator() noexcept = default;
	constexpr ForwardListIterator(node_base* pointer) noexcept : m_pointer(pointer) { }

	constexpr reference operator*() const { return static_cast<node_pointer>(m_pointer)->value; }
	constexpr pointer operator->() const noexcept { return &static_cast<node_pointer>(m_pointer)->value; }
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

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer == second.m_pointer;
	}

private:
	node_base* m_pointer;
};

} // namespace lsd
