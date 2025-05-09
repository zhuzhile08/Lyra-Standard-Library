/*************************
 * @file ForwardList.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Forward list implementation
 * 
 * @date 2024-03-28
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Iterators.h"
#include "Detail/ForwardListNode.h"

#include <initializer_list>
#include <functional>
#include <utility>

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

	using node_base = detail::ForwardListNodeBase;
	using node_type = detail::ForwardListNode<value_type>;
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

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept { return first.m_pointer == second.m_pointer; }

private:
	node_base* m_pointer;
};


template <class Ty, class Alloc = std::allocator<Ty>> class ForwardList {
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

	using iterator = ForwardListIterator<value_type>;
	using const_iterator = ForwardListIterator<const_value>; 

	using container = ForwardList;
	using container_reference = container&;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

private:
	using node_type = detail::ForwardListNode<value_type>;
	using node_pointer = node_type*;

	using node_alloc = allocator_traits::template rebind_alloc<node_type>;
	using node_traits = allocator_traits::template rebind_traits<node_type>;

public:

	constexpr ForwardList() = default;
	constexpr ForwardList(const_alloc_reference alloc) : m_alloc(alloc) { }
	constexpr ForwardList(size_type count, const_reference value, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc) { insertAfter(beforeBegin(), count, value); }
	constexpr ForwardList(size_type count, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc) { resize(count); }
	template <class It> constexpr ForwardList(It first, It last, const_alloc_reference alloc = allocator_type()) requires isIteratorValue<It> : 
		m_alloc(alloc) { insertAfter(beforeBegin(), first, last); }
	constexpr ForwardList(const_container_reference other, const_alloc_reference alloc = allocator_type()) :
		m_alloc(alloc) { insertAfter(beforeBegin(), other.begin(), other.end()); }
	constexpr ForwardList(container_rvreference other, const_alloc_reference alloc = allocator_type()) : 
		m_alloc(alloc), m_beforeHead(std::exchange(other.m_beforeHead, { })) { }
	constexpr ForwardList(init_list ilist, const_alloc_reference alloc = allocator_type()) :
		m_alloc(alloc) { insertAfter(beforeBegin(), ilist.begin(), ilist.end()); }
	
	constexpr ~ForwardList() {
		clear();
		m_beforeHead.next = nullptr;
	}

	constexpr void assign(size_type count, const_reference value) {
		clear();
		insertAfter(beforeBegin(), count, value);
	}
	template <class It> constexpr void assign(It first, It last) requires isIteratorValue<It> {
		clear();
		insertAfter(beforeBegin(), first, last);
	}
	constexpr void assign(init_list ilist) {
		assign(ilist.begin(), ilist.end());
	}

	constexpr ForwardList& operator=(const_container_reference other) {
		assign(other.begin(), other.end());
		return *this;
	}
	constexpr ForwardList& operator=(container_rvreference other) noexcept {
		m_alloc = std::move(other.m_alloc);
		m_beforeHead = std::exchange(other.m_beforeHead, nullptr);
		return *this;
	}
	constexpr ForwardList& operator=(init_list ilist) noexcept {
		assign(ilist.begin(), ilist.end());
		return *this;
	}

	constexpr void swap(container_reference other) noexcept {
		std::swap(m_alloc, other.m_alloc);
		std::swap(m_beforeHead.next, other.m_beforeHead.next);
	}

	[[nodiscard]] constexpr iterator beforeBegin() noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] constexpr const_iterator beforeBegin() const noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] constexpr const_iterator cbeforeBegin() const noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] [[deprecated]] constexpr iterator before_begin() noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] [[deprecated]] constexpr const_iterator before_begin() const noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] [[deprecated]] constexpr const_iterator cbefore_begin() const noexcept {
		return &m_beforeHead;
	}
	[[nodiscard]] constexpr iterator begin() noexcept {
		return m_beforeHead.next;
	}
	[[nodiscard]] constexpr const_iterator begin() const noexcept {
		return m_beforeHead.next;
	}
	[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
		return m_beforeHead.next;
	}
	[[nodiscard]] constexpr iterator end() noexcept {
		return nullptr;
	}
	[[nodiscard]] constexpr const_iterator end() const noexcept {
		return nullptr;
	}
	[[nodiscard]] constexpr const_iterator cend() const noexcept {
		return nullptr;
	}

	[[nodiscard]] constexpr reference front() {
		return *baseToNode(m_beforeHead.next)->value;
	}
	[[nodiscard]] constexpr const_reference front() const {
		return *baseToNode(m_beforeHead.next)->value;
	}

	constexpr iterator insertAfter(const_iterator pos, const_reference value) {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = it->next;
		
		it->next = node_traits::allocate(m_alloc, 1);
		node_traits::construct(m_alloc, baseToNode(it->next), value);

		it->next->next = next;

		return it->next;
	}
	constexpr iterator insertAfter(const_iterator pos, rvreference value) {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = it->next;
		
		it->next = node_traits::allocate(m_alloc, 1);
		node_traits::construct(m_alloc, baseToNode(it->next), std::move(value));

		it->next->next = next;

		return it->next;
	}
	constexpr iterator insertAfter(const_iterator pos, size_type count, const_reference value) {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = it->next;

		auto ptr = it;
		for (; count > 0; count--) {
			ptr->next = node_traits::allocate(m_alloc, 1);
			node_traits::construct(m_alloc, baseToNode(ptr->next), value);

			ptr = ptr->next;
		}

		ptr->next = next;

		return ptr;
	}
	template <class It> constexpr iterator insertAfter(const_iterator pos, It first, It last) requires isIteratorValue<It> {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = it->next;

		auto ptr = it;
		for (; first != last; first++) {
			ptr->next = node_traits::allocate(m_alloc, 1);
			node_traits::construct(m_alloc, baseToNode(ptr->next), *first);

			ptr = ptr->next;
		}

		ptr->next = next;

		return ptr;
	}
	constexpr iterator insertAfter(const_iterator pos, init_list ilist) {
		return insertAfter(pos, ilist.begin(), ilist.end());
	}
	template <class... Args> constexpr iterator emplaceAfter(const_iterator pos, Args&&... args) {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = it->next;
		
		it->next = node_traits::allocate(m_alloc, 1);
		node_traits::construct(m_alloc, baseToNode(it->next), std::forward<Args>(args)...);

		it->next->next = next;

		return it->next;
	}
	[[deprecated]] constexpr iterator insert_after(const_iterator pos, const_reference value) {
		return insertAfter(pos, value);
	}
	[[deprecated]] constexpr iterator insert_after(const_iterator pos, rvreference value) {
		return insertAfter(pos, std::move(value));
	}
	[[deprecated]] constexpr iterator insert_after(const_iterator pos, size_type count, const_reference value) {
		return insertAfter(pos, count, value);
	}
	template <class It> [[deprecated]] constexpr iterator insert_after(const_iterator pos, It first, It last) requires isIteratorValue<It> {
		return insertAfter(pos, first, last);
	}
	[[deprecated]] constexpr iterator insert_after(const_iterator pos, init_list ilist) {
		return insertAfter(pos, ilist.begin(), ilist.end());
	}
	template <class... Args> [[deprecated]] constexpr iterator emplace_after(const_iterator pos, Args&&... args) {
		return emplaceAfter(pos, std::forward<Args>(args)...);
	}

	constexpr void pushFront(const_reference value) {
		insertAfter(beforeBegin(), value);
	}
	constexpr void pushFront(rvreference value) {
		insertAfter(beforeBegin(), std::move(value));
	}
	template <class... Args> constexpr reference emplaceFront(Args&&... args) {
		return *emplaceAfter(beforeBegin(), std::forward<Args>(args)...);
	}
	[[deprecated]] constexpr void push_front(const_reference value) {
		return pushFront(value);
	}
	[[deprecated]] constexpr void push_front(rvreference value) {
		return pushFront(std::move(value));
	}
	template <class... Args> [[deprecated]] constexpr reference emplace_front(Args&&... args) {
		return emplaceFront(std::forward<Args>(args)...);
	}

	constexpr void popFront() {
		eraseAfter(beforeBegin());
	}
	[[deprecated]] constexpr void pop_front() {
		popFront();
	}

	constexpr void resize(size_type count) {
		size_type size = 0;
		--count; // Because count has to be subtracted everywhere else by 1 anyways
		for (auto it = begin(); it != end(); it++, size++) {
			if (size >= count) {
				eraseAfter(it, end());
				return;
			} else if (it.get()->next == nullptr && size < count) {
				insertAfter(it, count - size, { });
				return;
			}
		}
	}
	constexpr void resize(size_type count, const value_type& value) {
		size_type size = 0;
		--count; // Because count has to be subtracted everywhere else by 1 anyways
		for (auto it = begin(); it != end(); it++, size++) {
			if (size >= count) {
				eraseAfter(it, end());
				return;
			} else if (it.get()->next == nullptr && size < count) {
				insertAfter(it, count - size, value);
				return;
			}
		}
	}

	constexpr iterator eraseAfter(const_iterator pos) {
		auto it = const_cast<detail::ForwardListNodeBase*>(pos.get());
		auto next = std::exchange(it->next, it->next->next);

		node_traits::destroy(m_alloc, baseToNode(next));
		node_traits::deallocate(m_alloc, baseToNode(next), 1);

		return it->next;
	}
	constexpr iterator eraseAfter(const_iterator first, const_iterator last) {
		if (first != last) {
			auto fptr = const_cast<detail::ForwardListNodeBase*>(first.get());
			auto lptr = const_cast<detail::ForwardListNodeBase*>(last.get());
			auto next = std::exchange(fptr->next, lptr);

			while (next != lptr) {
				auto n = next->next;
				node_traits::destroy(m_alloc, baseToNode(next));
				node_traits::deallocate(m_alloc, baseToNode(next), 1);
				next = n;
			}

			return lptr;
		}

		return end();
	}
	[[deprecated]] constexpr iterator erase_after(const_iterator pos) {
		return eraseAfter(pos);
	}
	[[deprecated]] constexpr iterator erase_after(const_iterator first, const_iterator last) {
		return eraseAfter(first, last);
	}
	constexpr void clear() {
		eraseAfter(beforeBegin(), end());
	}

	[[nodiscard]] constexpr node_alloc getAllocator() const noexcept {
		return m_alloc;
	}
	[[nodiscard]] [[deprecated]] constexpr node_alloc get_allocator() const noexcept {
		return m_alloc;
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		if (m_beforeHead) return m_beforeHead == m_beforeHead.next;
		return true;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return node_traits::max_size(m_alloc);
	}
	[[nodiscard]] [[deprecated]] constexpr size_type max_size() const noexcept {
		return node_traits::max_size(m_alloc);
	}

private:
	[[no_unique_address]] node_alloc m_alloc { };
	detail::ForwardListNodeBase m_beforeHead { };

	static constexpr node_pointer baseToNode(detail::ForwardListNodeBase* base) noexcept {
		return static_cast<node_pointer>(base);
	}
};

}
