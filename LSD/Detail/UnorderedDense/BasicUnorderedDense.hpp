/*************************
 * @file BasicUnorderedDense.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation of both the unordered dense set and map
 * 
 * @date 2025-04-04
 * @copyright Copyright (c) 2025
 *************************/

#pragma once

#include "../CoreUtility.hpp"
#include "../../Iterators.h"
#include "../../Vector.h"
#include "../../Hash.h"

#include <cstdint>
#include <utility>
#include <limits>
#include <functional>
#include <initializer_list>
#include <type_traits>


namespace lsd {

namespace unordered_dense {

struct UnorderedDenseBucket {
public:
	using size_type = std::size_t;

	static constexpr size_type empty = std::numeric_limits<size_type>::max();

	size_type index = empty;
	size_type offset = 0;
};

template <class Key,
		 class Ty, // When Ty is void, treat as a set
		 class Hash,
		 class Equal,
		 class Alloc>
class BasicUnorderedDense {
public:
	// Type aliases and constants

	using key_type = Key;
	using mapped_type = Ty;
	using allocator_type = Alloc;
	template <class First, class Second> using pair_type = std::pair<First, Second>;

	using value_type = std::conditional_t<std::is_void_v<mapped_type>, key_type, pair_type<const key_type, mapped_type>>;
	using pointer = value_type*;
	using reference = value_type&;
	using const_reference = const value_type&;
	using rvreference = value_type&&;
	using array = Vector<value_type>;

	using size_type = typename array::size_type;
	using difference_type = typename array::difference_type;

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


private:
	static constexpr double maxLFactor = 0.8;
	static constexpr size_type bucketMinCount = 4;


	class WrappingIter {
	public:
		constexpr WrappingIter() = default;
		constexpr WrappingIter(const_iterator first) : first(first), iter(const_cast<pointer>(first.get())) { }

		constexpr bool nextToEnd(const_iterator end) {
			return iter != end && iter->index != bucket_type::empty;
		}

		constexpr bool beginToFirst() {
			return iter != first && iter->index != bucket_type::empty;
		}

		const const_bucket_iterator first { };
		bucket_iterator iter { };
	};

public:
	// Construction and assignment

	constexpr BasicUnorderedDense() : m_buckets(bucketMinCount) { }

	explicit constexpr BasicUnorderedDense(size_type bucketCount,
										   const hasher& hash = hasher(), 
										   const key_equal& keyEqual = key_equal(),
										   const allocator_type& alloc = allocator_type(),
										   const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : m_array(alloc),
		m_buckets(std::max(bucketMinCount, bucketCount), bucketAlloc), 
		m_hasher(hash), 
		m_equal(keyEqual) { }

	constexpr explicit BasicUnorderedDense(const allocator_type& alloc, const bucket_allocator_type& bucketAlloc = bucket_allocator_type()) 
		: BasicUnorderedDense(0, hasher(), key_equal(), alloc, bucketAlloc) { }
	constexpr BasicUnorderedDense(
		size_type bucketCount,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : BasicUnorderedDense(bucketCount, hasher(), key_equal(), alloc, bucketAlloc) { }
	constexpr BasicUnorderedDense(
		size_type bucketCount,
		const hasher& hasher,
		const allocator_type& alloc,
		const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : BasicUnorderedDense(bucketCount, hasher, key_equal(), alloc, bucketAlloc) { }


	template <std::input_iterator Iter> constexpr BasicUnorderedDense(Iter first, Iter last,
																	  size_type bucketCount = bucketMinCount,
																	  const hasher& hash = hasher(),
																	  const key_equal& keyEqual = key_equal(),
																	  const allocator_type& alloc = allocator_type(),
																	  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : m_array(first, last, alloc),
		m_buckets(std::max(std::max(bucketMinCount, bucketCount), m_array.capacity()), bucketAlloc),
		m_hasher(hash),
		m_equal(keyEqual)
	{
		basicRehash(m_buckets.size());
	}

	template <std::input_iterator It> constexpr BasicUnorderedDense(It first, It last,
																	size_type bucketCount,
																	const allocator_type& alloc,
																	const bucket_allocator_type& bucketAlloc = bucket_allocator_type())
		: BasicUnorderedDense(first, last, bucketCount, hasher(), key_equal(), alloc, bucketAlloc) { }
	template <std::input_iterator It> constexpr BasicUnorderedDense(It first, It last, 
																	size_type bucketCount,
																	const hasher& hash,
																	const allocator_type& alloc,
																	const bucket_allocator_type& bucketAlloc = bucket_allocator_type())
		: BasicUnorderedDense(first, last, bucketCount, hash, key_equal(), alloc, bucketAlloc) { }

	constexpr BasicUnorderedDense(init_list ilist, 
								  size_type bucketCount = bucketMinCount,
								  const hasher& hash = hasher(), 
								  const key_equal& keyEqual = key_equal(), 
								  const allocator_type& alloc = allocator_type(),
								  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : BasicUnorderedDense(ilist.begin(), ilist.end(), bucketCount, hash, keyEqual, alloc, bucketAlloc) { }
	constexpr BasicUnorderedDense(init_list ilist,
								  size_type bucketCount,
								  const allocator_type& alloc,
								  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : BasicUnorderedDense(ilist.begin(), ilist.end(), bucketCount, hasher(), key_equal(), alloc, bucketAlloc) { }
	constexpr BasicUnorderedDense(init_list ilist,
								  size_type bucketCount,
								  const hasher& hash,
								  const allocator_type& alloc,
								  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : BasicUnorderedDense(ilist.begin(), ilist.end(), bucketCount, hash, key_equal(), alloc, bucketAlloc) { }


	constexpr BasicUnorderedDense(const_container_reference other) :
		m_array(other.m_array), m_buckets(other.m_buckets) { }
	constexpr BasicUnorderedDense(const_container_reference other, 
								  const allocator_type& alloc, 
								  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : m_array(other.m_array, alloc), m_buckets(other.m_buckets, bucketAlloc) { }
	constexpr BasicUnorderedDense(container_rvreference other) noexcept : 
		m_array(std::move(other.m_array)), m_buckets(std::move(other.m_buckets)) { }
	constexpr BasicUnorderedDense(container_rvreference other,
								  const allocator_type& alloc,
								  const bucket_allocator_type& bucketAlloc = bucket_allocator_type()
	) : m_array(std::move(other.m_array), alloc), m_buckets(std::move(other.m_array), bucketAlloc) { }


	constexpr ~BasicUnorderedDense() = default;

	constexpr BasicUnorderedDense& operator=(const_container_reference other) noexcept {
		m_array = other.m_array;
		m_buckets = other.m_buckets;

		if constexpr (!std::is_empty_v<hasher>)
			m_hasher = other.m_hasher;
		if constexpr (!std::is_empty_v<key_equal>)
			m_equal = other.m_equal;

		return *this;
	}
	constexpr BasicUnorderedDense& operator=(container_rvreference other) noexcept {
		m_array = std::move(other.m_array);
		m_buckets = std::move(other.m_buckets);

		if constexpr (!std::is_empty_v<hasher>)
			m_hasher = std::move(other.m_hasher);
		if constexpr (!std::is_empty_v<key_equal>)
			m_equal = std::move(other.m_equal);

		return *this;
	}
	constexpr BasicUnorderedDense& operator=(init_list ilist) {
		clear();
		insert(ilist.begin(), ilist.end());

		return *this;
	}

	constexpr void swap(container& other) noexcept {
		std::swap(m_array, other.m_array);
		std::swap(m_buckets, other.m_buckets);

		if constexpr (!std::is_empty_v<hasher>)
			std::swap(m_hasher, other.m_hasher);
		if constexpr (!std::is_empty_v<key_equal>)
			std::swap(m_equal, other.m_equal);
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

	
	constexpr bucket_iterator bucketBegin() noexcept {
		return m_buckets.begin();
	}
	constexpr const_bucket_iterator bucketBegin() const noexcept {
		return m_buckets.cbegin();
	}
	constexpr const_bucket_iterator cbucketBegin() const noexcept {
		return m_buckets.cbegin();
	}
	constexpr bucket_iterator bucketEnd() noexcept {
		return m_buckets.end();
	}
	constexpr const_bucket_iterator bucketEnd() const noexcept {
		return m_buckets.cend();
	}
	constexpr const_bucket_iterator cbucketEnd() const noexcept {
		return m_buckets.cend();
	}

	[[deprecated]] constexpr bucket_iterator begin(size_type) noexcept {
		return bucketBegin();
	}
	[[deprecated]] constexpr const_bucket_iterator begin(size_type) const noexcept {
		return cbucketBegin();
	}
	[[deprecated]] constexpr const_bucket_iterator cbegin(size_type) const noexcept {
		return cbucketBegin();
	}
	[[deprecated]] constexpr bucket_iterator end(size_type) noexcept {
		return bucketEnd();
	}
	[[deprecated]] constexpr const_bucket_iterator end(size_type) const noexcept {
		return cbucketEnd();
	}
	[[deprecated]] constexpr const_bucket_iterator cend(size_type) const noexcept {
		return cbucketEnd();
	}

	
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
		m_buckets.clear();
		basicRehash(std::max(count, m_array.size() / maxLFactor));
	}

	constexpr void reserve(size_type count) {
		m_array.reserve(count);
		rehashIfNecessary(count);
	}


	// Insertion

	constexpr pair_type<iterator, bool> insert(const_reference value) {
		const auto baseIt = findBaseBucket(value);
		const auto it = find(baseIt, value);

		if (it != m_array.cend())
			return { it, false };
		return { insertBasic(baseIt, value), true };
	}
	template <class Value> constexpr pair_type<iterator, bool> insert(Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		const auto baseIt = findBaseBucket(value);
		const auto it = find(baseIt, value);

		if (it != m_array.end())
			return { it, false };
		return { insertBasic(baseIt, std::forward<Value>(value)), true };
	}
	constexpr iterator insert(const_iterator hint, const_reference value) {
		const auto it = find(hint, value);

		if (it != m_array.end())
			return { it, false };
		return { insertBasic(hint, value), true };
	}
	template <class Value> constexpr iterator insert(const_iterator hint, Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		const auto it = find(hint, value);

		if (it != m_array.end())
			return { it, false };
		return { insertBasic(hint, std::forward<Value>(value)), true };
	}

	template <std::input_iterator Iter> constexpr void insert(Iter first, Iter last) {
		if constexpr (std::contiguous_iterator<Iter>) {
			const auto dist = std::distance(first, last);
			m_array.reserve(dist);
		}

		for (; first != last; first++)
			insert(*first);
	}
	constexpr void insert(init_list ilist) {
		insert(ilist.begin(), ilist.end());
	}


	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		const auto baseIt = findBaseBucket(key);
		const auto it = find(baseIt, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(key, std::forward<V>(value)));
			
			return { it, false };
		} return { emplaceBasic(baseIt, key, std::forward<V>(value)), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		const auto baseIt = findBaseBucket(key);
		const auto it = find(baseIt, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::forward<K>(key), std::forward<V>(value)));
			
			return { it, false };
		} return { emplaceBasic(baseIt, std::forward<K>(key), std::forward<V>(value)), true };
	}
	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		const auto it = find(hint, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(key, std::forward<V>(value)));
			
			return { it, false };
		} return { emplaceBasic(hint, key, std::forward<V>(value)), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		const auto it = find(hint, key);

		if (it != m_array.end()) {
			*it = std::move(value_type(std::forward<K>(key), std::forward<V>(value)));
			
			return { it, false };
		} return { emplaceBasic(hint, std::forward<K>(key), std::forward<V>(value)), true };
	}

	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(key, std::forward<V>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(K&& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}
	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, const key_type& key, V&& value) requires(!std::is_void_v<mapped_type>) {
		return insertOrAssign(hint, key, std::forward<V>(value));
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
		const auto baseIt = findBaseBucket(key);
		const auto it = find(baseIt, key);

		if (it != m_array.end())
			return { it, false };
		return { emplaceBasic(baseIt, key, std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		const auto baseIt = findBaseBucket(key);
		const auto it = find(baseIt, key);

		if (it != m_array.end())
			return { it, false };
		return { emplaceBasic(baseIt, std::forward<K>(key), std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		const auto it = find(hint, key);

		if (it != m_array.end())
			return { it, false };
		return { emplaceBasic(hint, key, std::forward<Args>(args)...), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		const auto it = find(hint, key);

		if (it != m_array.end())
			return { it, false };
		return { emplaceBasic(hint, std::forward<K>(key), std::forward<Args>(args)...), true };
	}

	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		return tryEmplace(key, args...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		return tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, const key_type& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		return tryEmplace(hint, key, args...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, K&& key, Args&&... args) requires(!std::is_void_v<mapped_type>) {
		return tryEmplace(hint, std::forward<K>(key), std::forward<Args>(args)...);
	}


	template <class H2, class E2> constexpr void merge(BasicUnorderedDense<key_type, mapped_type, H2, E2, allocator_type>& source) noexcept {
		insert(source.begin(), source.end());
	}
	template <class H2, class E2> constexpr void merge(BasicUnorderedDense<key_type, mapped_type, H2, E2, allocator_type>&& source) noexcept {
		insert(source.begin(), source.end());
	}


	// Erasure

	constexpr iterator erase(const_iterator pos) {
		assert((pos != m_array.end()) && "lsd::UnorderedDenseMap::erase(): Illegal past end iterator!");

		const auto index = pos - m_array.cbegin();
		for (auto bucketIt = findBaseBucket(*pos); bucketIt != m_buckets.cend(); bucketIt++) {
			if (bucketIt->index == index) {
				eraseBucket(bucketIt);

				break;
			}
		}


		if (m_array.size() == 1) {
			m_array.popBack();
			return;
		}

		// Instead of erasing the element with vector.erase, the last element gets shifted to the erased position

		iterator it { const_cast<pointer>(pos.get()) };
		*it = std::move(m_array.back());
		m_array.popBack();
		
		const auto base = findBaseBucket(*it);

		for (auto bucketIt = base; bucketIt != m_buckets.end(); bucketIt++) {
			if (bucketIt->index == m_array.size()) { // array size corresponds with the old largest index of the array
				bucketIt->index = index;

				return it;
			}
		}

		for (auto bucketIt = m_buckets.begin(); bucketIt != base; bucketIt++) {
			if (bucketIt->index == m_array.size()) {
				bucketIt->index = index;

				break;
			}
		}

		return it;
	}
	constexpr iterator erase(iterator pos) {
		return erase(const_iterator { pos });
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		for (; first != last; first++)
			erase(first);
		return m_array.begin() + (last - first);
	}
	constexpr size_type erase(const key_type& key) {
		if (auto it = find(key); it != m_array.end()) {
			erase(it);
			return 1;
		}

		return 0;
	}
	template <class K> constexpr size_type erase(K&& key) {
		if (auto it = find(std::forward<K>(key)); it != m_array.end()) {
			erase(it);
			return 1;
		}

		return 0;
	}

	constexpr value_type extract(const_iterator pos) {
		assert((pos != m_array.end()) && "lsd::BasicUnorderedDense::extract(): Illegal end iterator was passed to the function!");

		value_type res(std::move(*const_cast<value_type*>(pos.get())));
		erase(pos);
		return res;
	}
	constexpr value_type extract(const key_type& key) {
		if (auto it = find(key); it != m_array.end())
			return extract(it);
		return value_type();
	} 
	template <class K> constexpr value_type extract(K&& key) {
		if (auto it = find(std::forward<K>(key)); it != m_array.end())
			return extract(it);
		return value_type();
	}

	constexpr void clear() {
		m_array.clear();
		m_buckets.clear();
	}


	// Lookup
	
	template <class K> [[nodiscard]] constexpr iterator find(const K& key) noexcept {
		const auto base = findBaseBucket(key);

		for (auto it = base; it != m_buckets.end() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.begin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key))
				return arrIt;
		}

		for (auto it = m_buckets.begin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.begin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key))
				return arrIt;
		}

		return m_array.end();
	}
	template <class K> [[nodiscard]] constexpr const_iterator find(const K& key) const noexcept {
		const auto base = findCBaseBucket(key);

		for (auto it = base; it != m_buckets.cend() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.cbegin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else
				if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key)) return arrIt;
		}

		for (auto it = m_buckets.cbegin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.cbegin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else if (auto arrIt = m_array.cbegin() + it->index; m_equal(arrIt->first, key))
				return arrIt;
		}

		return m_array.cend();
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

	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) noexcept {
		return equalRange(key);
	}
	template <class K> [[deprecated]] [[nodiscard]] constexpr auto equal_range(const K& key) const noexcept {
		return equalRange(key);
	}


	template <class K> [[nodiscard]] constexpr bool contains(const K& key) const noexcept {
		auto base = findCBaseBucket(key);

		for (auto it = base; it != m_buckets.cend() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key)) return true;
			} else
				if (m_equal(m_array[it->index].first, key)) return true;
		}

		for (auto it = m_buckets.cbegin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key)) return true;
			} else
				if (m_equal(m_array[it->index].first, key)) return true;
		}

		return false;
	}

	template <class K> [[nodiscard]] constexpr size_type count(const K& key) const noexcept {
		auto base = findCBaseBucket(key);

		for (auto it = base; it != m_buckets.cend() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key)) return 1;
			} else
				if (m_equal(m_array[it->index].first, key)) return 1;
		}

		for (auto it = m_buckets.cbegin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key)) return 1;
			} else
				if (m_equal(m_array[it->index].first, key)) return 1;
		}

		return 0;
	}


	template <class K> [[nodiscard]] constexpr auto& at(const K& key) {
		auto it = find(key);
		if (it == m_array.end()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>)
			return *it;
		else
			return it->second;
	}
	template <class K> [[nodiscard]] constexpr const auto& at(const K& key) const {
		auto it = find(key);
		if (it == m_array.cend()) throw std::out_of_range("lsd::BasicUnorderedDense::at(): Specified key could not be found in container!");

		if constexpr (std::is_void_v<mapped_type>)
			return *it;
		else
			return it->second;
	}

	[[nodiscard]] constexpr auto& operator[](const key_type& key) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if constexpr (std::is_void_v<mapped_type>)
			return (it == m_array.end()) ? *insertBasic(baseIt, key) : *it;
		else
			return (it == m_array.end()) ? emplaceBasic(baseIt, key, mapped_type())->second : it->second;
	}
	template <class K> [[nodiscard]] constexpr auto& operator[](K&& key) {
		auto baseIt = findBaseBucket(key);
		auto it = find(baseIt, key);

		if constexpr (std::is_void_v<mapped_type>)
			return (it == m_array.end()) ? *insertBasic(baseIt, std::forward<K>(key)) : *it;
		else
			return (it == m_array.end()) ? emplaceBasic(baseIt, std::forward<K>(key), mapped_type())->second : it->second;
	}

	
	template <class K> [[nodiscard]] const bucket_type& bucket(const K& key) const {
		auto base = findCBaseBucket(key);

		for (auto it = base; it != m_buckets.cend() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key))
					return *it;
			} else if (m_equal(m_array[it->index].first, key))
				return *it;
		}

		for (auto it = m_buckets.cbegin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (m_equal(m_array[it->index], key))
					return *it;
			} else if (m_equal(m_array[it->index].first, key))
				return *it;
		}

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
		return allocator();
	}

	[[nodiscard]] constexpr size_type bucketSize() const noexcept {
		return m_buckets.size();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type bucket_count() const noexcept {
		return bucketSize();
	}
	[[nodiscard]] constexpr size_type maxBucketSize() const noexcept {
		return m_buckets.maxSize();
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_bucket_count() const noexcept {
		return maxBucketSize();
	}
	[[nodiscard]] consteval size_type bucketSize(size_type index) const noexcept {
		return m_buckets[index] == bucket_type::empty;
	}
	[[deprecated]] [[nodiscard]] consteval size_type bucket_size(size_type index) const noexcept {
		return bucketSize(index);
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
		return maxLFactor;
	}

private:
	array m_array { };
	buckets m_buckets { };

	[[no_unique_address]] hasher m_hasher { };
	[[no_unique_address]] key_equal m_equal { };


	// Utility and base functions

	// Value based find functions
	
	constexpr bucket_iterator findBaseBucket(const value_type& value) noexcept {
		if constexpr (std::is_void_v<mapped_type>)
			return m_buckets.begin() + (m_hasher(value) % m_buckets.size());
		else
			return m_buckets.begin() + (m_hasher(value.first) % m_buckets.size());
	}

	[[nodiscard]] constexpr iterator find(const_bucket_iterator it, const value_type& value) noexcept
		requires (!std::is_void_v<mapped_type>)
	{
		const auto base = it;

		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++) {
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, value.first))
				return arrIt;
		}

		for (it = m_buckets.begin(); it != base && it->index != bucket_type::empty; it++) {
			if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, value.first))
				return arrIt;
		}

		return m_array.end();
	}

	// Key based find functions

	template <class K> constexpr bucket_iterator findBaseBucket(const K& key) noexcept {
		return m_buckets.begin() + (m_hasher(key) % m_buckets.size());
	}
	template <class K> constexpr const_bucket_iterator findCBaseBucket(const K& key) const noexcept {
		return m_buckets.cbegin() + (m_hasher(key) % m_buckets.size());
	}

	template <class K> [[nodiscard]] constexpr iterator find(const_bucket_iterator it, const K& key) noexcept {
		const auto base = it;

		for (; it != m_buckets.end() && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.begin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key))
				return arrIt;
		}

		for (it = m_buckets.begin(); it != base && it->index != bucket_type::empty; it++) {
			if constexpr (std::is_void_v<mapped_type>) {
				if (auto arrIt = m_array.begin() + it->index; m_equal(*arrIt, key))
					return arrIt;
			} else if (auto arrIt = m_array.begin() + it->index; m_equal(arrIt->first, key))
				return arrIt;
		}

		return m_array.end();
	}


	// Bucket insert

	constexpr void insertBucket(bucket_iterator it, size_type index) noexcept {
		const const_bucket_iterator base { it };
		const auto bucketEnd = m_buckets.cend();

		size_type offset { };
		do {
			do { // m_buckets.end() will never be passed to this function
				if (it->index == bucket_type::empty) {
					*it = bucket_type { index, offset };

					return;
				}

				if (offset > it->offset) {
					bucket_type tmp = *it;
					*it = { index, offset };

					index = tmp.index;
					offset = tmp.offset;
				}

				++offset;
			} while (it != bucketEnd);

			it = m_buckets.begin();

			// In theory, iteration should stop once it has reached its original position, but that is
			// guaranteed to happen as there will always be at least the same amount of buckets as elements
		} while (true);
	}
	constexpr void eraseBucket(bucket_iterator it) noexcept {
		const const_bucket_iterator base { it };
		const auto target = m_buckets.cend();

		*it = bucket_type { };

		do {
			for (auto last = it++; it != target && it->index != bucket_type::empty && it->offset > 0; it++, last++) {
				*last = *it;
				--(last->offset);
			}

			it = m_buckets.begin();
		} while (true);
	}


	// Rehash

	constexpr void basicRehash(size_type count) {
		try {
			m_buckets.resize(std::min(count, m_buckets.maxSize()));
		} catch(std::length_error) {
			throw std::length_error("lsd::unordered_dense::BasicUnorderedDense::basicRehash(): Size exceeded maximum possible size!");
		}

		size_type index = 0;
		const auto arrayEnd = m_array.cend();
		for (auto it = m_array.cbegin(); it != arrayEnd; it++, index++)
			insertBucket(findBaseBucket(*it), index);
	}
	constexpr bool rehashIfNecessary(size_type count) {
		// count represents the requested element count, whilst bucket size * max load factor represents the ideal element count
		if (count >= m_buckets.size() * maxLFactor) {
			m_buckets.clear();

			basicRehash(std::max(
				m_buckets.size() * 2,
				static_cast<size_type>(count / maxLFactor)
			));

			return true;
		}

		return false;
	}


	// Insert and Emplace

	constexpr iterator insertBasic(bucket_iterator bucketIt, const value_type& value) {
		if (rehashIfNecessary(m_array.size() + 1))
			insertBucket(findBaseBucket(value), m_array.size());
		else
			insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(value);
	}
	constexpr iterator insertBasic(bucket_iterator bucketIt, value_type&& value) {
		if (rehashIfNecessary(m_array.size() + 1))
			insertBucket(findBaseBucket(value), m_array.size());
		else
			insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(std::move(value));
	}
	template <class K, class... Args> constexpr iterator emplaceBasic(bucket_iterator bucketIt, K&& key, Args&&... args)
		requires (!std::is_void_v<mapped_type>)
	{
		if (rehashIfNecessary(m_array.size() + 1))
			insertBucket(findBaseBucket(key), m_array.size());
		else
			insertBucket(bucketIt, m_array.size());

		return &m_array.emplaceBack(std::forward<K>(key), mapped_type(std::forward<Args>(args)...));
	}
};

} // namespace unordered_dense

} // namespace lsd
