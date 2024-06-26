/*************************
 * @file Node.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 *
 * @brief Base node class
 *
 * @date 2022-22-06
 *
 * @copyright Copyright (c) 2022
 *************************/

#pragma once

#include "UniquePointer.h"
#include "SharedPointer.h"
#include "Hash.h"
#include "UnorderedSparseSet.h"

#include <type_traits>
#include <utility>

#include "String.h"
#include "StringView.h"

namespace lsd {

template <class, class = void> struct IsHashMap : public std::false_type { };
template <class Ty> struct IsHashMap<Ty, std::void_t<typename Ty::hasher>> : public std::true_type { };
template <class Ty> static constexpr inline bool isHashMapValue = IsHashMap<Ty>::value;

template <
	class Type, 
	template <class...> class SmartPointer = UniquePointer,
	class Key = String,
	template <class...> class Container = UnorderedSparseSet> 
class BasicNode {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using value_type = Type;
	using const_value = const value_type;
	using reference = value_type&;
	using const_reference = const_value&;
	using movable = value_type&&;
	using pointer = value_type*;
	using const_pointer = const pointer;
	using pointer_const = const_value*;
	using smart_pointer = SmartPointer<value_type>;

	using key_type = Key;
	using key_reference = key_type&;
	using const_key_reference = const key_type&;
	using key_rvreference = key_type&&;

private:
	CUSTOM_HASHER(Hasher, const smart_pointer&, const_key_reference, Hash<key_type>{}, ->m_name)
	CUSTOM_EQUAL(Equal, const smart_pointer&, const_key_reference, ->m_name)

public:
	using container = Container<smart_pointer, Hasher, Equal>;

	using iterator = typename container::iterator;
	using const_iterator = typename container::const_iterator;
	using iterator_pair = std::pair<iterator, bool>;

	static_assert(isHashMapValue<container> && std::is_same_v<typename container::value_type, smart_pointer>, "lsd::BasicNode: Container template parameter only supports key-only hash maps. Please make sure the container is a hash map and has the type alias hasher defined!");

	constexpr BasicNode() = default;
	template <class KeyType> explicit constexpr BasicNode(const KeyType& name) : m_name(name) { }
	template <class KeyType> explicit constexpr BasicNode(KeyType&& name) : m_name(std::forward<KeyType>(name)) { }
	constexpr BasicNode(BasicNode&&) = default;
	constexpr BasicNode(const BasicNode&) requires std::is_copy_assignable_v<smart_pointer> = default;
	virtual constexpr ~BasicNode() = default;

	constexpr BasicNode& operator=(BasicNode&&) = default;
	constexpr BasicNode& operator=(const BasicNode&) requires std::is_copy_assignable_v<smart_pointer> = default;

	constexpr reference clear() noexcept { 
		m_children.clear();
		return *this;
	}

	constexpr iterator begin() noexcept {
		return m_children.begin();
	}
	constexpr const_iterator begin() const noexcept {
		return m_children.begin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return m_children.cbegin();
	}

	constexpr iterator end() noexcept {
		return m_children.end();
	}
	constexpr const_iterator end() const noexcept {
		return m_children.end();
	}
	constexpr const_iterator cend() const noexcept {
		return m_children.cend();
	}

	constexpr reference insert(movable child) {
		child.m_parent = this;
		return *m_children.emplace(smart_pointer::create(std::move(child))).first->get();
	}
	constexpr reference insert(smart_pointer&& child) {
		child->m_parent = this;
		return *m_children.emplace(std::move(child)).first->get();
	}
	template <class... Args> constexpr reference emplace(Args&&... args) {
		auto child = m_children.emplace(smart_pointer::create(std::forward<Args>(args)...)).first->get();
		child->m_parent = this;
		return *child;
	}

	template <class KeyType> constexpr reference rename(KeyType&& name) {
		auto t = dynamic_cast<pointer>(this);
		m_parent->m_children.extract(m_name);
		m_name = std::forward<KeyType>(name);
		m_parent->m_children.emplace(smart_pointer(t));
		return *t;
	}

	constexpr reference erase(iterator pos) { 
		return m_children.erase(pos); 
		return *dynamic_cast<pointer>(this);
	}
	constexpr reference erase(const_iterator pos) { 
		return m_children.erase(pos); 
		return *dynamic_cast<pointer>(this);
	}
	constexpr reference erase(const_iterator first, const_iterator last) { 
		return m_children.erase(first, last); 
		return *dynamic_cast<pointer>(this);
	}
	template <class KeyType> constexpr size_type erase(KeyType&& name) requires std::is_convertible_v<KeyType, key_type> { 
		return m_children.erase(std::forward(name)); 
	}

	constexpr void swap(reference other) noexcept { 
		m_children.swap(other.m_children); 
	}
	[[deprecated]] constexpr void swap(container& other) noexcept { 
		m_children.swap(other); 
	}

	[[nodiscard]] constexpr bool empty() const noexcept { 
		return m_children.empty(); 
	}
	constexpr operator bool() const noexcept { 
		return m_children.empty(); 
	}

	template <class KeyType> constexpr iterator find(KeyType&& name) { 
		return m_children.find(std::forward<KeyType>(name)); 
	}
	template <class KeyType> constexpr const_iterator find(KeyType&& name) const { 
		return m_children.find(std::forward<KeyType>(name)); 
	}
	template <class KeyType> constexpr bool contains(KeyType&& name) const { 
		return m_children.contains(std::forward<KeyType>(name)); 
	}

	template <class KeyType> constexpr const_reference child(KeyType&& key) const {
		constexpr bool stringlike = requires(const KeyType& k) {
			key_type(key);
			key_type().find("");
			key_type().substr(0, 0);
		};

		if constexpr (stringlike) {
			key_type k(key);
			size_type beg = 0, cur;
			pointer_const p = dynamic_cast<pointer_const>(this);

			while ((cur = k.find("::", beg)) < k.size()) {
				p = p->m_children.at(k.substr(beg, cur - beg));
				(beg = cur) += 2;
			}

			return *p->m_children.at(k.substr(beg)).get();
		} else {
			return *m_children.at(key).get();
		}
	}
	template <class KeyType> constexpr reference child(KeyType&& key) {
		constexpr bool stringlike = requires(const KeyType& k) {
			key_type(key);
			key_type().find("");
			key_type().substr(0, 0);
		};

		if constexpr (stringlike) {
			key_type k(key);
			size_type beg = 0, cur;
			pointer p = dynamic_cast<pointer>(this);

			while ((cur = k.find("::", beg)) < k.size()) {
				p = p->m_children.at(k.substr(beg, cur - beg));
				beg = cur + 2;
			}

			return *p->m_children.at(k.substr(beg)).get();
		} else {
			return *m_children.at(key).get();
		}
	}

	template <class KeyType> constexpr const_reference at(KeyType&& name) const { *m_children.at(std::forward<KeyType>(name)); }
	template <class KeyType> constexpr reference at(KeyType&& name) { *m_children.at(std::forward<KeyType>(name)); }
	
	template <class KeyType> constexpr const_reference operator[](KeyType&& name) const { return *m_children[std::forward<KeyType>(name)]; }
	template <class KeyType> constexpr reference operator[](KeyType&& name) { return *m_children[std::forward<KeyType>(name)]; }

	[[nodiscard]] constexpr size_type size() const noexcept { return m_children.size(); }
	[[nodiscard]] constexpr const_key_reference name() const noexcept { return m_name; }
	[[nodiscard]] constexpr const_pointer const parent() const noexcept { return dynamic_cast<pointer>(m_parent); }

protected:
	key_type m_name { };

	BasicNode* m_parent = nullptr;
	container m_children { };

	friend struct HashFunction;
	friend struct EqualFunction;
};

template <class Ty> using Node = BasicNode<Ty, UniquePointer>;
template <class Ty> using SharedNode = BasicNode<Ty, SharedPointer>;

} // namespace lsd
