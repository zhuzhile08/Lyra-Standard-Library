/*************************
 * @file BasicUnorderedDense.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation of both the Unordered dense set and map
 * 
 * @date 2025-04-04
 * @copyright Copyright (c) 2025
 *************************/

#pragma once

#include "CoreUtility.h"
#include "../Iterators.h"
#include "../Vector.h"
#include "../Array.h"
#include "../Hash.h"

#include <cstdint>
#include <utility>
#include <limits>
#include <functional>
#include <initializer_list>
#include <type_traits>

#define LSD_DETAIL_MULTI_FUNCTION_REDEFINITION(signature, camelCase, snakeCase, args, body, params) signature camelCase args body signature snakeCase args { camelCase(params) }

namespace lsd {

namespace detail {

struct UnorderedDenseBucket {
public:
	using size_type = std::size_t;

	static constexpr size_type empty = std::numeric_limits<size_type>::max();

	size_type index = empty;
	size_type offset = 0;
};

template <
	class Key,
	class Ty, // When Ty is void, treat as a set
	class Hash,
	class Equal,
	class Alloc
> class BasicUnorderedDense {
public:
	// Type aliases and constants
	
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	template <class First, class Second> using pair_type = std::pair<First, Second>;

	static constexpr float maxLFactor = 0.9;
	static constexpr size_type bucketMinCount = 4;

	using key_type = Key;
	using mapped_type = Ty;
	using allocator_type = Alloc;

	using value_type = std::conditional_t<std::is_void_v<mapped_type>, key_type, pair_type<key_type, mapped_type>>;
	using reference = value_type&;
	using const_reference = const value_type&;
	using rvreference = value_type&&;
	using array = Vector<value_type>;

	using bucket_type = UnorderedDenseBucket;
	using bucket_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<bucket_type>;
	using buckets = Vector<bucket_type, bucket_allocator_type>;

	using iterator = typename array::iterator;
	using const_iterator = typename array::const_iterator;
	using reverse_iterator = typename array::reverse_iterator;
	using reverse_const_iterator = typename array::const_reverse_iterator;

	using bucket_iterator = typename buckets::iterator;
	using const_bucket_iterator = typename buckets::const_iterator;

	using hasher = Hash;
	using key_equal = Equal;

	using container = BasicUnorderedDense;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;


	// Construction and assignment

	constexpr BasicUnorderedDense() noexcept : m_buckets(bucketMinCount) { }

	explicit constexpr BasicUnorderedDense(
		size_type bucketCount, 
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) noexcept :
		m_array(alloc),
		m_buckets(std::min(std::max(bucketMinCount, bucketCount), bucket_type::empty - 1), bucketAlloc), 
		m_hasher(hash), 
		m_equal(keyEqual) { } 
	constexpr BasicUnorderedDense(
		size_type bucketCount,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) noexcept :
		m_array(alloc),
		m_buckets(std::min(std::max(bucketMinCount, bucketCount), bucket_type::empty - 1), bucketAlloc) { }
	constexpr BasicUnorderedDense(
		size_type bucketCount,
		const hasher& hasher,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) noexcept :
		m_array(alloc),
		m_buckets(std::min(std::max(bucketMinCount, bucketCount), bucket_type::empty - 1), bucketAlloc),
		m_hasher(hasher) { }
	constexpr BasicUnorderedDense(const allocator_type& alloc, const bucket_allocator_type& bucketAlloc) noexcept : 
		m_array(alloc), m_buckets(bucketMinCount, bucketAlloc) {
	}
	
	template <ContinuousIteratorType It> constexpr BasicUnorderedDense(
		It first, It last, 
		size_type bucketCount = bucketMinCount,
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : m_array(alloc),
		m_buckets(std::max(bucketCount, std::distance(first, last)), bucketAlloc), 
		m_hasher(hash), 
		m_equal(keyEqual)
	{
		insert(first, last);
	}
	template <ContinuousIteratorType It> constexpr BasicUnorderedDense(
		It first, It last,
		size_type bucketCount,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) : m_array(alloc), m_buckets(std::max(bucketCount, std::distance(first, last)), bucketAlloc) {
		insert(first, last);
	}
	template <ContinuousIteratorType It> constexpr BasicUnorderedDense(
		It first, It last, 
		size_type bucketCount,
		const hasher& hasher,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) : m_array(alloc),
		m_buckets(std::max(bucketCount, std::distance(first, last)), bucketAlloc), 
		m_hasher(hasher)
	{
		insert(first, last);
	}
		
	constexpr BasicUnorderedDense(const_container_reference other) :
		m_array(other.m_array), m_buckets(other.m_buckets) { }
	constexpr BasicUnorderedDense(const_container_reference other, const allocator_type& alloc, const bucket_allocator_type& bucketAlloc) :
		m_array(other.m_array, alloc), m_buckets(other.m_buckets, bucketAlloc) { }
	constexpr BasicUnorderedDense(container_rvreference other) noexcept : 
		m_array(std::move(other.m_array)), m_buckets(std::move(other.m_buckets)) { }
	constexpr BasicUnorderedDense(container_rvreference other, const allocator_type& alloc, const bucket_allocator_type& bucketAlloc) : 
		m_array(std::move(other.m_array), alloc), m_buckets(std::move(other.m_array), bucketAlloc) { }
	
	constexpr BasicUnorderedDense(
		init_list ilist, 
		size_type bucketCount = bucketMinCount,
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) :
		m_array(alloc),
		m_buckets(std::max(bucketCount, ilist.size()), bucketAlloc),
		m_hasher(hash),
		m_equal(keyEqual)
	{
		insert(ilist.begin(), ilist.end());
	}
	constexpr BasicUnorderedDense(
		init_list ilist,
		size_type bucketCount,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) : m_array(alloc), m_buckets(std::max(bucketCount, ilist.size()), bucketAlloc) {
		insert(ilist.begin(), ilist.end());
	} 
	constexpr BasicUnorderedDense(
		init_list ilist,
		size_type bucketCount,
		const hasher& hasher,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc
	) : m_array(alloc), m_buckets(std::max(bucketCount, ilist.size()), bucketAlloc), m_hasher(hasher) {
		insert(ilist.begin(), ilist.end());
	}

	constexpr ~BasicUnorderedDense() = default;

	constexpr BasicUnorderedDense& operator=(const_container_reference other) noexcept {
		m_array = other.m_array;
		m_buckets = other.m_buckets;

		if constexpr (!std::is_empty_v<hasher>) m_hasher = other.m_hasher;
		if constexpr (!std::is_empty_v<key_equal>) m_equal = other.m_equal;
	}
	constexpr BasicUnorderedDense& operator=(container_rvreference other) noexcept {
		m_array = std::move(other.m_array);
		m_buckets = std::move(other.m_buckets);

		if constexpr (!std::is_empty_v<hasher>) m_hasher = std::move(other.m_hasher);
		if constexpr (!std::is_empty_v<key_equal>) m_equal = std::move(other.m_equal);
	}
	constexpr BasicUnorderedDense& operator=(init_list ilist) {
		insert(ilist.begin(), ilist.end());
	}

	constexpr void swap(container& other) noexcept {
		std::swap(m_array, other.m_array);
		std::swap(m_buckets, other.m_buckets);

		if constexpr (!std::is_empty_v<hasher>) std::swap(m_hasher, other.m_hasher);
		if constexpr (!std::is_empty_v<key_equal>) std::swap(m_equal, other.m_equal);
	}


	// Iterator functions

	constexpr iterator begin() noexcept {
		return m_array.begin();
	}
	constexpr const_iterator begin() const noexcept {
		return m_array.begin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return m_array.cbegin();
	}
	constexpr iterator end() noexcept {
		return m_array.end();
	}
	constexpr const_iterator end() const noexcept {
		return m_array.end();
	}
	constexpr const_iterator cend() const noexcept {
		return m_array.cend();
	}
	constexpr reverse_iterator rbegin() noexcept {
		return m_array.rbegin();
	}
	constexpr reverse_const_iterator rbegin() const noexcept {
		return m_array.rbegin();
	}
	constexpr reverse_const_iterator crbegin() const noexcept {
		return m_array.crbegin();
	}
	constexpr reverse_iterator rend() noexcept {
		return m_array.rend();
	}
	constexpr reverse_const_iterator rend() const noexcept {
		return m_array.rend();
	}
	constexpr reverse_const_iterator crend() const noexcept {
		return m_array.crend();
	}

	constexpr bucket_iterator begin(size_type) noexcept = delete;
	constexpr const_bucket_iterator begin(size_type) const noexcept = delete;
	constexpr const_bucket_iterator cbegin(size_type) const noexcept = delete;
	constexpr bucket_iterator end(size_type) noexcept = delete;
	constexpr const_bucket_iterator end(size_type) const noexcept = delete;
	constexpr const_bucket_iterator cend(size_type) const noexcept = delete;

	
	[[nodiscard]] constexpr reference front() noexcept {
		return m_array.front();
	}
	[[nodiscard]] constexpr const_reference front() const noexcept {
		return m_array.front();
	}
	[[nodiscard]] constexpr reference back() noexcept {
		return m_array.back();
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return m_array.back();
	}


	// Capacity modification

	constexpr void rehash(size_type count) {
		if (count >= m_array.size() / maxLFactor) basicRehash(count);
	}

	constexpr void reserve(size_type count) {
		m_array.reserve(count);
		rehashIfNecessary(count);
	}


	// Insertion

	constexpr pair_type<iterator, bool> insert(const_reference value) {
		if constexpr (std::is_void_v<mapped_type>) {
			auto baseIt = findBaseBucket(value);
			auto it = find(baseIt, value);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(baseIt, value), true };
		} else {
			auto baseIt = findBaseBucket(value.first);
			auto it = find(baseIt, value.first);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(baseIt, value), true };
		}
	}
	template <class Value> constexpr pair_type<iterator, bool> insert(Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		if constexpr (std::is_void_v<mapped_type>) {
			auto baseIt = findBaseBucket(value);
			auto it = find(baseIt, value);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(baseIt, std::forward<Value>(value)), true };
		} else {
			auto baseIt = findBaseBucket(value.first);
			auto it = find(baseIt, value.first);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(baseIt, std::forward<Value>(value)), true };
		}
	}
	constexpr iterator insert(const_iterator hint, const_reference value) {
		if constexpr (std::is_void_v<mapped_type>) {
			auto it = findWrap(hint, value);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(hint, value), true };
		} else {
			auto it = findWrap(hint, value.first);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(hint, value), true };
		}
	}
	template <class Value> constexpr iterator insert(const_iterator hint, Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		if constexpr (std::is_void_v<mapped_type>) {
			auto it = findWrap(hint, value);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(hint, std::forward<Value>(value)), true };
		} else {
			auto it = findWrap(hint, value.first);

			if (it != m_array.end()) return { it, false };
			return { basicInsert(hint, std::forward<Value>(value)), true };
		}
	}

	template <IteratorType It> constexpr void insert(It first, It last) {
		for (; first < last; first++)
			insert(*first);
	}
	constexpr void insert(init_list ilist) {
		insert(ilist.begin(), ilist.end());
	}


	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(key, std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(baseIt, key, std::forward<V>(value)), true };
	}
	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(key_type&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::move(key), std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(baseIt, std::move(key), std::forward<V>(value)), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::forward<K>(key), std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(baseIt, std::forward<K>(key), std::forward<V>(value)), true };
	}
	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(key, std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(hint, key, std::forward<V>(value)), true };
	}
	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, key_type&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::move(key), std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(hint, std::move(key), std::forward<V>(value)), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::forward<K>(key), std::forward<V>(value)));
			
			return { it, false };
		} return { basicEmplace(hint, std::forward<K>(key), std::forward<V>(value)), true };
	}

	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(key, std::forward<V>(value));
	}
	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(key_type&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(std::move(key), std::forward<V>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}
	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(hint, key, std::forward<V>(value));
	}
	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, key_type&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(hint, move(key), std::forward<V>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(hint, std::forward<K>(key), std::forward<V>(value));
	}


	template <class... Args> constexpr pair_type<iterator, bool> emplace(Args&&... args) noexcept {
		return insert(value_type(std::forward<Args>(args)...));
	}
	template <class... Args> constexpr iterator emplaceHint(const_iterator hint, Args&&... args) noexcept {
		return insert(hint, value_type(std::forward<Args>(args)...));
	}
	template <class... Args> [[deprecated]] constexpr iterator emplace_hint(const_iterator hint, Args&&... args) noexcept {
		return emplaceHint(hint, std::forward<Args>(args)...);
	}


	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(baseIt, key, std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(key_type&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(baseIt, std::move(key), std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(baseIt, std::forward<K>(key), std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(hint, key, std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, key_type&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(hint, std::move(key), std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		auto it = findWrap(hint, key);

		if (it != m_array.end()) return { it, false };
		return { basicEmplace(hint, std::forward<K>(key), std::forward<Args>(args)...), true };
	}

	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(key, args...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(key_type&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(std::move(key), std::move(args)...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(hint, key, args...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, key_type&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(hint, std::move(key), std::move(args)...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		tryEmplace(hint, std::forward<K>(key), std::forward<Args>(args)...);
	}


	template <class H2, class E2> constexpr void merge(BasicUnorderedDense<key_type, mapped_type, H2, E2, allocator_type>& source) noexcept {
		insert(source.begin(), source.end());
	}
	template <class H2, class E2> constexpr void merge(BasicUnorderedDense<key_type, mapped_type, H2, E2, allocator_type>&& source) noexcept {
		insert(source.begin(), source.end());
	}


	// Erasure

	constexpr iterator erase(const_iterator pos) noexcept {
		assert((pos != m_array.end()) && "lsd::UnorderedDenseMap::erase(): Illegal end iterator was passed to the function!");

		auto index = pos - m_array.cbegin();
		auto it = m_array.begin() + index;

		if constexpr (std::is_void_v<mapped_type>) {
			for (auto it = findBaseBucket(*pos); it != m_buckets.cend(); it++) {
				if (it->index == index) {
					eraseBucket(it);

					break;
				}
			}
		} else {
			for (auto it = findBaseBucket(pos->first); it != m_buckets.cend(); it++) {
				if (it->index == index) {
					eraseBucket(it);

					break;
				}
			}
		}

		*it = std::move(*m_array.rbegin());
		m_array.popBack();

		return it;
	}
	constexpr iterator erase(iterator pos) noexcept {
		return erase(const_iterator { pos });
	}
	constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
		for (; first != last; first++) erase(first);
		return m_array.begin() + (last - first);
	}
	constexpr size_type erase(const key_type& key) noexcept {
		if (auto it = find(key); it != m_array.end()) {
			erase(it);
			return 1;
		}

		return 0;
	}
	template <class K> constexpr size_type erase(K&& key) noexcept {
		if (auto it = find(std::forward<K>(key)); it != m_array.end()) {
			erase(it);
			return 1;
		}

		return 0;
	}

	constexpr value_type extract(const_iterator pos) noexcept {
		assert((pos != m_array.end()) && "lsd::BasicUnorderedDense::extract(): Illegal end iterator was passed to the function!");

		value_type res(std::move(*const_cast<value_type*>(pos.get())));
		erase(pos);
		return res;
	}
	constexpr value_type extract(const key_type& key) noexcept {
		if (auto it = find(key); it != m_array.end()) return extract(it);
		return value_type();
	} 
	template <class K> constexpr value_type extract(K&& key) noexcept {
		if (auto it = find(std::forward<K>(key)); it != m_array.end()) return extract(it);
		return value_type();
	}

	constexpr void clear() noexcept {
		m_array.clear();
		m_buckets.clear();
	}


	// Lookup
	
	[[nodiscard]] constexpr iterator find(const key_type& key) noexcept {
		for (auto it = findBaseBucket(key); it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;
		
		return m_array.end();
	}
	[[nodiscard]] constexpr const_iterator find(const key_type& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key)) return arrIt;
		
		return m_array.cend();
	}
	template <class K> [[nodiscard]] constexpr iterator find(const K& key) noexcept {
		for (auto it = findBaseBucket(key); it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;
		
		return m_array.end();
	}
	template <class K> [[nodiscard]] constexpr const_iterator find(const K& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key)) return arrIt;
		
		return m_array.cend();
	}

	[[nodiscard]] constexpr pair_type<iterator, iterator> equalRange(const key_type& key) noexcept {
		auto it = find(key);

		if (it == m_array.end()) return { it, it };
		return { it, it + 1 };
	}
	[[nodiscard]] constexpr pair_type<const_iterator, const_iterator> equalRange(const key_type& key) const noexcept {
		auto it = find(key);

		if (it == m_array.end()) return { it, it };
		return { it, it + 1 };
	}
	template <class K> [[nodiscard]] constexpr pair_type<iterator, iterator> equalRange(const K& key) noexcept {
		auto it = find(key);

		if (it == m_array.end()) return { it, it };
		return { it, it + 1 };
	}
	template <class K> [[nodiscard]] constexpr pair_type<const_iterator, const_iterator> equalRange(const K& key) const noexcept {
		auto it = find(key);

		if (it == m_array.end()) return { it, it };
		return { it, it + 1 };
	}

	[[deprecated]] [[nodiscard]] constexpr auto equal_range(const key_type& key) noexcept {
		return equalRange(key);
	}
	[[deprecated]] [[nodiscard]] constexpr auto equal_range(const key_type& key) const noexcept {
		return equalRange(key);
	}
	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) noexcept {
		return equalRange(key);
	}
	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) const noexcept {
		return equalRange(key);
	}


	[[nodiscard]] constexpr bool contains(const key_type& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (m_equal(m_array[it->index].first, key)) return true;
		
		return false;
	}
	template <class K> [[nodiscard]] constexpr bool contains(const K& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (m_equal(m_array[it->index].first, key)) return true;
		
		return false;
	}

	[[nodiscard]] constexpr size_type count(const key_type& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (m_equal(m_array[it->index].first, key)) return 1;
		
		return 0;
	}
	template <class K> [[nodiscard]] constexpr size_type count(const K& key) const noexcept {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (m_equal(m_array[it->index].first, key)) return 1;
		
		return 0;
	}


	[[nodiscard]] constexpr auto& at(const key_type& key) {
		auto it = find(key);
		if (it == m_array.end()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>) return *it;
		else return it->second;
	}
	[[nodiscard]] constexpr const auto& at(const key_type& key) const {
		auto it = find(key);
		if (it == m_array.cend()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>) return *it;
		else return it->second;
	}
	template <class K> [[nodiscard]] constexpr auto& at(const K& key) {
		auto it = find(key);
		if (it == m_array.end()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>) return *it;
		else return it->second;
	}
	template <class K> [[nodiscard]] constexpr const auto& at(const K& key) const {
		auto it = find(key);
		if (it == m_array.cend()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>) return *it;
		else return it->second;
	}

	[[nodiscard]] constexpr auto& operator[](const key_type& key) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if constexpr (std::is_void_v<mapped_type>)
			return (it == m_array.end()) ? *basicInsert(baseIt, key) : *it;
		else
			return (it == m_array.end()) ? basicEmplace(baseIt, key, mapped_type())->second : it->second;
	}
	[[nodiscard]] constexpr auto& operator[](key_type&& key) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if constexpr (std::is_void_v<mapped_type>)
			return (it == m_array.end()) ? *basicInsert(baseIt, std::move(key)) : *it;
		else
			return (it == m_array.end()) ? basicEmplace(baseIt, std::move(key), mapped_type())->second : it->second;
	}
	template <class K> [[nodiscard]] constexpr auto& operator[](K&& key) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if constexpr (std::is_void_v<mapped_type>)
			return (it == m_array.end()) ? *basicInsert(baseIt, std::forward<K>(key)) : *it;
		else
			return (it == m_array.end()) ? basicEmplace(baseIt, std::forward<K>(key), mapped_type())->second : it->second;
	}

	
	[[nodiscard]] const bucket_type& bucket(const key_type& key) const {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key)) return *it;

		throw std::out_of_range("lsd::BasicUnorderedDense::bucket(): Bucket pointing to element with requested key doesn't exist!");
		return m_buckets.front();
	}
	template <class K> [[nodiscard]] const bucket_type& bucket(const K& key) const {
		for (auto it = findCBaseBucket(key); it != m_buckets.cend() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key)) return *it;

		throw std::out_of_range("lsd::BasicUnorderedDense::bucket(): Bucket pointing to element with requested key doesn't exist!");
		return m_buckets.front();
	}


	// Getters

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_array.size();
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return m_array.maxSize() - 1;
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_array.empty();
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_array.allocator();
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return m_array.allocator();
	}

	[[nodiscard]] constexpr size_type bucketSize() const noexcept {
		return m_buckets.size();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type bucket_count() const noexcept {
		return bucketSize();
	}
	[[nodiscard]] constexpr size_type maxBucketSize() const noexcept {
		return m_buckets.maxSize() - 1;
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_bucket_count() const noexcept {
		return maxBucketSize();
	}
	[[deprecated]] [[nodiscard]] consteval size_type bucket_size(size_type) const noexcept {
		return 1;
	}

	[[nodiscard]] constexpr bucket_allocator_type bucketAllocator() const noexcept {
		return m_buckets.allocator();
	}


	[[nodiscard]] constexpr hasher hashFunction() const noexcept {
		return m_hasher;
	}
	[[deprecated]] [[nodiscard]] constexpr hasher hash_function() const noexcept {
		return hashFunction();
	}
	[[nodiscard]] constexpr key_equal keyEq() const noexcept {
		return m_equal;
	}
	[[deprecated]] [[nodiscard]] constexpr hasher key_eq() const noexcept {
		return keyEq();
	}


	[[nodiscard]] constexpr float loadFactor() const noexcept {
		return m_array.size() / m_buckets.size();
	}
	[[deprecated]] [[nodiscard]] constexpr float load_factor() const noexcept {
		return loadFactor();
	}
	[[nodiscard]] consteval float maxLoadFactor() const noexcept {
		return maxLFactor;
	}
	[[deprecated]] [[nodiscard]] consteval float max_load_factor() const noexcept {
		return maxLoadFactor();
	}

private:
	array m_array { };
	buckets m_buckets { };

	[[no_unique_address]] hasher m_hasher { };
	[[no_unique_address]] key_equal m_equal { };


	// Utility and base functions

	constexpr bucket_iterator findBaseBucket(const key_type& key) noexcept {
		return m_buckets.begin() + (m_hasher(key) % m_buckets.size());
	}
	constexpr const_bucket_iterator findCBaseBucket(const key_type& key) const noexcept {
		return m_buckets.cbegin() + (m_hasher(key) % m_buckets.size());
	}
	template <class K> constexpr bucket_iterator findBaseBucket(const K& key) noexcept {
		return m_buckets.begin() + (m_hasher(key) % m_buckets.size());
	}
	template <class K> constexpr const_bucket_iterator findCBaseBucket(const K& key) const noexcept {
		return m_buckets.cbegin() + (m_hasher(key) % m_buckets.size());
	}

	[[nodiscard]] constexpr iterator find(const_bucket_iterator it, const key_type& key) noexcept {
		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		return m_array.end();
	}
	[[nodiscard]] constexpr iterator findWrap(const_bucket_iterator it, const key_type& key) noexcept {
		auto base = it;
		
		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		for (it = m_buckets.begin(); it != base; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		return m_array.end();
	}
	template <class K> [[nodiscard]] constexpr iterator find(const_bucket_iterator it, const K& key) noexcept {
		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		return m_array.end();
	}
	template <class K> [[nodiscard]] constexpr iterator findWrap(const_bucket_iterator it, const K& key) noexcept {
		auto base = it;
		
		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		for (it = m_buckets.begin(); it != base; it++)
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key)) return arrIt;

		return m_array.end();
	}


	constexpr void insertBucket(bucket_iterator it, size_type index, size_type offset = 0) {
		auto base = it;
		do {
			if (it->index == bucket_type::empty) {
				*it = bucket_type { index, offset };

				return;
			}

			if (offset > it->offset) {
				std::swap(index, it->index);
				std::swap(offset, it->offset);
			}

			++offset;
			if (++it == m_buckets.end()) it = m_buckets.begin();
		} while (it != base);
	}
	constexpr void eraseBucket(bucket_iterator it) noexcept {
		auto current = it;
		
		do {
			if (++it == m_buckets.end())
				it = m_buckets.begin();

			*current = *it;
			--current->offset;

			current = it;
		} while (it->index != bucket_type::empty && it->offset > 0);

		*it = bucket_type { };
	}

	constexpr void basicRehash(size_type count) {
		if (count == bucket_type::empty) throw std::length_error("lsd::BasicUnorderedDense::basicRehash(): Requested rehash size larger than the maximum size of the container!");

		m_buckets.clear();
		m_buckets.resize(count);

		size_type index = 0;

		if constexpr (std::is_void_v<mapped_type>)
			for (auto it = m_array.cbegin(); it != m_array.cend(); it++, index++)
				insertBucket(findBaseBucket(*it), index);
		else for (auto it = m_array.cbegin(); it != m_array.cend(); it++, index++)
				insertBucket(findBaseBucket(it->first), index);
	}
	constexpr bool rehashIfNecessary(size_type count) {
		if (m_buckets.size() != m_buckets.maxSize() - 1 && count > m_buckets.size() * maxLFactor) {
			auto countMax = count / maxLFactor; // Emulates constexpr ceil
			auto trunc = static_cast<size_type>(countMax);

			basicRehash(std::max(
				m_buckets.size() * 2,
				(countMax > trunc) ? trunc + 1 : trunc
			));

			return true;
		}

		return false;
	}


	constexpr iterator basicInsert(bucket_iterator bucketIt, const value_type& value) {
		if (rehashIfNecessary(m_array.size() + 1)) {
			if constexpr (std::is_void_v<mapped_type>) insertBucket(findBaseBucket(value), m_array.size());
			else insertBucket(findBaseBucket(value.first), m_array.size());
		} else insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(value);
	}
	constexpr iterator basicInsert(bucket_iterator bucketIt, value_type&& value) {
		if (rehashIfNecessary(m_array.size() + 1)) {
			if constexpr (std::is_void_v<mapped_type>) insertBucket(findBaseBucket(value), m_array.size());
			else insertBucket(findBaseBucket(value.first), m_array.size());
		} else insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(std::move(value));
	}
	template <class... Args> constexpr iterator basicEmplace(bucket_iterator bucketIt, key_type&& key, Args&&... args)
		requires(!std::is_void_v<mapped_type>)
	{
		if (rehashIfNecessary(m_array.size() + 1)) insertBucket(findBaseBucket(key), m_array.size());
		else insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(std::move(key), mapped_type(std::forward<Args>(args)...));
	}
	template <class K, class... Args> constexpr iterator basicEmplace(bucket_iterator bucketIt, K&& key, Args&&... args)
		requires(!std::is_void_v<mapped_type>)
	{
		if (rehashIfNecessary(m_array.size() + 1)) insertBucket(findBaseBucket(key), m_array.size());
		else insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(std::forward<K>(key), mapped_type(std::forward<Args>(args)...));
	}
};

} // namespace detail

} // namespace lsd
