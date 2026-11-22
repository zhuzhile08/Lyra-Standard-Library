/*************************
 * @file BasicUnorderedFlat.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation of both the unordered flat set and map
 * 
 * @date 2025-04-06
 * @copyright Copyright (c) 2025
 *************************/

#pragma once

#include "UnorderedFlatCore.hpp"
#include "Metadata.hpp"
#include "Iterator.hpp"

#include <utility>
#include <initializer_list>
#include <type_traits>


namespace lsd {

namespace unordered_flat {

template <class Key,
		  class Ty, // When Ty is void, treat as a set
		  class Hash,
		  class Equal,
		  class Alloc>
class BasicUnorderedFlat {
public:
	// Type aliases and constants
	
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	template <class First, class Second> using pair_type = std::pair<First, Second>;

	using key_type = Key;
	using mapped_type = Ty;
	using allocator_type = Alloc;
	using allocator_traits = std::allocator_traits<allocator_type>;

	using value_type = std::conditional_t<std::is_void_v<mapped_type>, key_type, pair_type<key_type, mapped_type>>;
	using const_value = const value_type;
	using pointer = value_type*;
	using reference = value_type&;
	using const_pointer = const_value*;
	using const_reference = const_value&;
	using rvreference = value_type&&;

	using metadata_group = detail::MetadataGroup;
	using metadata_pointer = metadata_group*;
	using metadata_allocator_type = allocator_traits::template rebind_alloc<metadata_group>;
	using metadata_allocator_traits = allocator_traits::template rebind_traits<metadata_group>;

	using iterator = unordered_flat::Iterator<value_type>;
	using const_iterator = unordered_flat::Iterator<const value_type>;

	using hasher = Hash;
	using key_equal = Equal;

	using container = BasicUnorderedFlat;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

	using container_core = UnorderedFlatCore<key_type, mapped_type, hasher, key_equal, Alloc>;

public:
	// Construction and assignment

	constexpr BasicUnorderedFlat() : m_container() { }

	explicit constexpr BasicUnorderedFlat(size_type bucketCount, 
										  const hasher& hash = hasher(), 
										  const key_equal& keyEqual = key_equal(), 
										  const allocator_type& alloc = allocator_type(),
										  const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) : 
		m_container(bucketCount, hash, keyEqual, alloc, metadataAlloc) { }
	constexpr BasicUnorderedFlat(size_type bucketCount,
								 const allocator_type& alloc,
								 const metadata_allocator_type& metadataAlloc) :
		m_container(bucketCount, hasher(), key_equal(), alloc, metadataAlloc) { }
	constexpr BasicUnorderedFlat(size_type bucketCount,
								 const hasher& hasher,
								 const allocator_type& alloc,
								 const metadata_allocator_type& metadataAlloc) :
		m_container(bucketCount, hasher, key_equal(), alloc, metadataAlloc) { }
	constexpr BasicUnorderedFlat(const allocator_type& alloc,
								 const metadata_allocator_type& metadataAlloc) : 
		m_container(size_type { 1 }, hasher(), key_equal(), alloc, metadataAlloc) { }

	template <std::input_iterator Iter> constexpr BasicUnorderedFlat(Iter first, Iter last, 
																	 size_type bucketCount = container_core::bucketMinCount,
																	 const hasher& hash = hasher(), 
																	 const key_equal& keyEqual = key_equal(), 
																	 const allocator_type& alloc = allocator_type(),
																	 const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) : 
		m_container(first, last, bucketCount, hash, keyEqual, alloc, metadataAlloc) { }
	template <std::input_iterator Iter> constexpr BasicUnorderedFlat(Iter first, Iter last,
																	 size_type bucketCount,
																	 const allocator_type& alloc,
																	 const metadata_allocator_type& metadataAlloc) :
		m_container(first, last, bucketCount, hasher(), key_equal(), alloc, metadataAlloc) { }
	template <std::input_iterator Iter> constexpr BasicUnorderedFlat(Iter first, Iter last, 
																	 size_type bucketCount,
																	 const hasher& hash,
																	 const allocator_type& alloc,
																	 const metadata_allocator_type& metadataAlloc) :
		m_container(first, last, hash, key_equal(), alloc, metadataAlloc) { }

	constexpr BasicUnorderedFlat(const_container_reference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		m_container(other.m_container.begin(), other.m_container.end(), other.m_container.bucketCount(), hasher(), key_equal(), alloc, metadataAlloc) { }
	
	constexpr BasicUnorderedFlat(const_container_reference other) : 
		m_container(other.m_container.begin(),
					other.m_container.end(),
					other.m_container.bucketCount(),
					hasher(), key_equal(), 
					allocator_traits::select_on_container_copy_construction(other.m_container.alloc),
					metadata_allocator_traits::select_on_container_copy_construction(other.m_container.metadataAlloc)) { }

	constexpr BasicUnorderedFlat(container_rvreference other) noexcept : m_container(std::move(other.m_container)) { }
	constexpr BasicUnorderedFlat(container_rvreference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		m_container(std::move(other.m_container)) { }

	constexpr BasicUnorderedFlat(init_list ilist, 
								 size_type bucketCount = container_core::bucketMinCount,
								 const hasher& hash = hasher(), 
								 const key_equal& keyEqual = key_equal(), 
								 const allocator_type& alloc = allocator_type(),
								 const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) :
		m_container(ilist.begin(), ilist.end(), bucketCount, hash, keyEqual, alloc, metadataAlloc) { }
	constexpr BasicUnorderedFlat(init_list ilist,
								 size_type bucketCount,
								 const allocator_type& alloc,
								 const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) : 
		m_container(ilist.begin(), ilist.end(), bucketCount, hasher(), key_equal(), alloc, metadataAlloc) { }
	constexpr BasicUnorderedFlat(init_list ilist,
								 size_type bucketCount,
								 const hasher& hash,
								 const allocator_type& alloc,
								 const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) :
		m_container(ilist.begin(), ilist.end(), bucketCount, hash, key_equal(), alloc, metadataAlloc) { }	



	constexpr BasicUnorderedFlat& operator=(const_container_reference other) noexcept {
		m_container = other.m_container;
		return *this;
	}
	constexpr BasicUnorderedFlat& operator=(container_rvreference other) noexcept {
		m_container = std::move(other.m_container);
		return *this;
	}
	constexpr BasicUnorderedFlat& operator=(init_list ilist) {
		m_container = ilist;
		return *this;
	}


	constexpr void swap(container& other) noexcept {
		m_container.swap(other.m_container);
	}


	// Iterator functions

	constexpr iterator begin() noexcept {
		return m_container.begin();
	}
	constexpr const_iterator begin() const noexcept {
		return m_container.cbegin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return m_container.cbegin();
	}
	constexpr iterator end() noexcept {
		return m_container.end();
	}
	constexpr const_iterator end() const noexcept {
		return m_container.cend();
	}
	constexpr const_iterator cend() const noexcept {
		return m_container.cend();
	}


	// Capacity modification

	constexpr void rehash(size_type bucketCount) {
		m_container.rehash(bucketCount);
	}
	constexpr void reserve(size_type count) {
		m_container.reserve(count);
	}


	// Insertion

	constexpr pair_type<iterator, bool> insert(const_reference value) {
		return m_container.insert(value);
	}
	template <class Value> constexpr pair_type<iterator, bool> insert(Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		return m_container.insert(std::forward<Value>(value));
	}
	constexpr iterator insert(const_iterator, const_reference value) {
		return m_container.insert(value).first;
	}
	template <class Value> constexpr iterator insert(const_iterator, Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		return m_container.insert(std::forward<Value>(value)).first;
	}

	template <std::input_iterator Iter> constexpr void insert(Iter first, Iter last) {
		m_container.insert(first, last);
	}
	constexpr void insert(init_list ilist) {
		return m_container.insert(ilist.begin(), ilist.end());
	}


	template <class Value>
	constexpr pair_type<iterator, bool> insertOrAssign(const key_type& key, Value&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.insertOrAssign(key, std::forward<Value>(value));
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER {
		return m_container.insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}
	template <class Value>
	constexpr iterator insertOrAssign(const_iterator, const key_type& key, Value&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.insertOrAssign(key, std::forward<Value>(value)).first;
	}
	template <class K, class V>
	constexpr iterator insertOrAssign(const_iterator, K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.insertOrAssign(std::forward<K>(key), std::forward<V>(value)).first;
	}

	template <class Value>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const key_type& key, Value&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(key, std::forward<Value>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER {
		return insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}
	template <class Value>
	[[deprecated]] constexpr iterator insert_or_assign(const_iterator, const key_type& key, Value&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(key, std::forward<Value>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr iterator insert_or_assign(const_iterator, K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}


	template <class... Args> constexpr pair_type<iterator, bool> emplace(Args&&... args) noexcept {
		return m_container.insert(value_type(std::forward<Args>(args)...));
	}
	template <class... Args> constexpr iterator emplaceHint(const_iterator, Args&&... args) noexcept {
		return m_container.insert(value_type(std::forward<Args>(args)...)).first;
	}
	template <class... Args> [[deprecated]] constexpr iterator emplace_hint(const_iterator, Args&&... args) noexcept {
		return emplaceHint(std::forward<Args>(args)...);
	}


	template <class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.tryEmplace(key, std::forward<Args>(args)...);
	}
	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER {
		return m_container.tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}
	template <class... Args>
	constexpr iterator tryEmplace(const_iterator, const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.tryEmplace(key, std::forward<Args>(args)...).first;
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator, K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return m_container.tryEmplace(std::forward<K>(key), std::forward<Args>(args)...).first;
	}

	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return tryEmplace(key, std::forward<Args>(args)...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER {
		return tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator, const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return tryEmplace(key, std::forward<Args>(args)...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator, K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}


	template <class H2, class E2> constexpr void merge(BasicUnorderedFlat<key_type, mapped_type, H2, E2, allocator_type>& source) noexcept {
		m_container.merge(source.m_container);
	}


	// Erasure

	constexpr iterator erase(iterator pos) {
		return m_container.erase(pos);
	}
	constexpr iterator erase(const_iterator pos) {
		return m_container.erase(iterator { pos.m_metadata, const_cast<pointer>(pos.m_pointer) });
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		return m_container.erase(first, last);
	}
	template <class K> constexpr size_type erase(K&& key) {
		if (const auto it = find(std::forward<K>(key)); it.m_pointer != nullptr) {
			erase(it);
			return 1;
		}

		return 0;
	}
	constexpr size_type erase(const key_type& key) {
		return erase(key);
	}

	constexpr value_type extract(const_iterator pos) {
		assert((pos.m_pointer != nullptr) && "lsd::BasicUnorderedFlat::extract(): Invalid iterator was passed to the function!");

		value_type res(std::move(*const_cast<value_type*>(pos.get())));
		erase(pos); // Erase does not rely on the key or value, only the data of the iterator, therefore this is safe
		return res;
	}
	template <class K> constexpr value_type extract(K&& key) requires(!std::is_convertible_v<K&&, const_iterator>) {
		if (auto it = find(std::forward<K>(key)); it.m_pointer != nullptr)
			return extract(it);
		return value_type();
	}
	constexpr value_type extract(const key_type& key) {
		return extract(key);
	}

	constexpr void clear() noexcept {
		m_container.clear();
	}


	// Lookup

	template <class K> [[nodiscard]] constexpr iterator find(const K& key) noexcept {
		const auto hash = m_container.postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = m_container.hashToGroup(hash);

		return m_container.template find<true>(shortHash, groupIndex, key);
	}
	template <class K> [[nodiscard]] constexpr const_iterator find(const K& key) const noexcept {
		const auto hash = m_container.postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = m_container.hashToGroup(hash);

		return m_container.template find<true>(shortHash, groupIndex, key);
	}

	template <class K> [[nodiscard]] constexpr pair_type<iterator, iterator> equalRange(const K& key) noexcept {
		auto it = find(key);

		if (it.m_pointer == nullptr) return { { } };
		return { it, it + 1 };
	}
	template <class K> [[nodiscard]] constexpr pair_type<const_iterator, const_iterator> equalRange(const K& key) const noexcept {
		auto it = find(key);

		if (it.m_pointer == nullptr) return { { } };
		return { it, it + 1 };
	}

	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) noexcept {
		return equalRange(key);
	}
	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) const noexcept {
		return equalRange(key);
	}


	template <class K> [[nodiscard]] constexpr bool contains(const K& key) const noexcept {
		const auto hash = m_container.postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = m_container.hashToGroup(hash);

		return m_container.template find<false>(shortHash, groupIndex, key) != nullptr;
	}

	template <class K> [[nodiscard]] constexpr size_type count(const K& key) const noexcept {
		return contains(key) ? 1 : 0;
	}


	template <class K> [[nodiscard]] constexpr auto& at(const K& key) {
		const auto it = find(key);
		if (it.m_pointer == nullptr)
			throw std::out_of_range("lsd::BasicUnorderedFlat::at(): Specified key could not be found in container!");

		return m_container.pairToValue(*it);
	}
	template <class K> [[nodiscard]] constexpr const auto& at(const K& key) const {
		const auto it = find(key);
		if (it.m_pointer == nullptr)
			throw std::out_of_range("lsd::BasicUnorderedFlat::at(): Specified key could not be found in container!");

		return m_container.pairToValue(*it);
	}

	[[nodiscard]] constexpr auto& operator[](const key_type& key) {
		return m_container[key];
	}
	template <class K> [[nodiscard]] constexpr auto& operator[](K&& key) {
		return m_container[std::forward<K>(key)];
	}


	// Getters

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_container.size;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return maxBucketCount() * maxLoadFactor();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_container.size == 0;
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_container.alloc;
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return allocator();
	}

	[[nodiscard]] constexpr size_type bucketGroupSize() const noexcept {
		return m_container.bucketGroupSize();
	}
	[[nodiscard]] constexpr size_type bucketCount() const noexcept {
		return m_container.bucketCount();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type bucket_count() const noexcept {
		return bucketCount();
	}
	[[nodiscard]] constexpr size_type maxBucketCount() const noexcept {
		return m_container.maxBucketCount();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_bucket_count() const noexcept {
		return maxBucketCount();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type bucket_size(size_type) const noexcept {
		return 1;
	}

	[[nodiscard]] constexpr size_type groupSize() const noexcept {
		return m_container.groupSize();
	}
	[[nodiscard]] constexpr size_type maxGroupCount() const noexcept {
		return m_container.maxGroupCount();
	}
	[[nodiscard]] constexpr metadata_allocator_type metadataAllocator() const noexcept {
		return m_container.metadataAlloc;
	}


	[[nodiscard]] constexpr hasher hashFunction() const noexcept {
		return m_container.hashFunction;
	}
	[[deprecated]] [[nodiscard]] constexpr hasher hash_function() const noexcept {
		return hashFunction();
	}
	[[nodiscard]] constexpr key_equal keyEq() const noexcept {
		return m_container.equal;
	}
	[[deprecated]] [[nodiscard]] constexpr hasher key_eq() const noexcept {
		return keyEq();
	}


	[[nodiscard]] constexpr float loadFactor() const noexcept {
		return m_container.loadFactor();
	}
	[[deprecated]] [[nodiscard]] constexpr float load_factor() const noexcept {
		return loadFactor();
	}
	[[nodiscard]] constexpr float maxLoadFactor() const noexcept {
		return m_container.maxLoadFactor();
	}
	[[deprecated]] [[nodiscard]] constexpr float max_load_factor() const noexcept {
		return maxLoadFactor();
	}


	friend constexpr bool operator==(const_container_reference lhs, const_container_reference rhs) {
		if (lhs.m_container.size != rhs.m_container.size)
			return false;

		if LSD_UNORDERED_FLAT_IS_SET {
			for (const auto& value : lhs.m_container)
				if (!rhs.contains(value))
					return false;
		} else {
			for (const auto& [key, value] : lhs.m_container) {
				const auto it = rhs.find(key);
				if (!it.get())
					return false;

				if (it->second != value)
					return false;
			}
		}

		return true;
	}

private:
	container_core m_container;
};

} // namespace unordered_flat

} // namespace lsd
