/************************
 * @file UnorderedFlatCore.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core implementation of the unordered maps
 * 
 * @date 2026-11-13
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "Metadata.hpp"
#include "Iterator.hpp"
#include "../../Hash.h"

#include <cstdint>
#include <utility>
#include <bit>
#include <initializer_list>
#include <type_traits>


#define LSD_UNORDERED_FLAT_IS_SET constexpr (std::is_void_v<mapped_type>)
#define LSD_UNORDERED_FLAT_REQUIRES_MAP requires(!std::is_void_v<mapped_type>)
#define LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER requires(!std::is_void_v<mapped_type> && !std::is_convertible_v<K&&, const_iterator> && !std::is_convertible_v<K&&, iterator>)

namespace lsd {

namespace unordered_flat {

template <class Key,
		  class Ty, // When Ty is void, treat as a set
		  class Hash,
		  class Equal,
		  class Alloc>
class UnorderedFlatCore {
public:
	// Type aliases and constants
	
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	template <class First, class Second> using pair_type = std::pair<First, Second>;

	static constexpr double maxLFactor = 0.875;
	static constexpr double invLFactor = 1.0 / maxLFactor;
	static constexpr size_type bucketMinCount = 2;

	using key_type = Key;
	using mapped_type = Ty;
	using allocator_type = Alloc;
	using allocator_traits = std::allocator_traits<allocator_type>;

	using value_type = std::conditional_t<std::is_void_v<mapped_type>, key_type, pair_type<key_type, mapped_type>>;
	using true_mapped_type = std::conditional_t<std::is_void_v<mapped_type>, key_type, mapped_type>;
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

	using container = UnorderedFlatCore;
	using const_container_reference = const container&;
	using container_rvreference = container&&;
	using init_list = std::initializer_list<value_type>;

public:
	// Construction and assignment

	constexpr UnorderedFlatCore() {
		basicRehash(bucketMinCount);
	}

	explicit constexpr UnorderedFlatCore(size_type bucketCount, 
										 const hasher& hash = hasher(), 
										 const key_equal& keyEqual = key_equal(), 
										 const allocator_type& alloc = allocator_type(),
										 const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) : 
		hashFunction(hash), equal(keyEqual), alloc(alloc), metadataAlloc(metadataAlloc)
	{
		basicRehash(std::max(bucketMinCount, bucketCount));
	}

	template <std::input_iterator Iter> constexpr UnorderedFlatCore(Iter first, Iter last, 
																	size_type bucketCount = bucketMinCount,
																	const hasher& hash = hasher(), 
																	const key_equal& keyEqual = key_equal(), 
																	const allocator_type& alloc = allocator_type(),
																	const metadata_allocator_type& metadataAlloc = metadata_allocator_type()) : 
		hashFunction(hash), equal(keyEqual), alloc(alloc), metadataAlloc(metadataAlloc)
	{
		if constexpr (std::contiguous_iterator<Iter>)
			basicRehash(std::max(static_cast<size_type>(std::distance(first, last) * invLFactor), std::max(bucketMinCount, bucketCount)));
		else
			basicRehash(std::max(bucketMinCount, bucketCount));

		for (; first != last; first++)
			insert(std::move(*first));
	}

	constexpr UnorderedFlatCore(container_rvreference other) noexcept :
		metadata(std::exchange(other.metadata, nullptr)),
		array(std::exchange(other.array, nullptr)),
		size(std::exchange(other.size, 0)),
		groupCount(std::exchange(other.groupCount, 0)),
		m_counterToRehash(other.m_counterToRehash),
		hashFunction(std::move(other.hashFunction)),
		equal(std::move(other.equal)),
		alloc(std::move(other.alloc)),
		metadataAlloc(std::move(other.metadataAlloc)) { }
	constexpr UnorderedFlatCore(container_rvreference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc)
		requires(allocator_traits::is_always_equal::value && metadata_allocator_traits::is_always_equal::value) : 
			metadata(std::exchange(other.metadata, nullptr)),
			array(std::exchange(other.array, nullptr)),
			size(std::exchange(other.size, 0)),
			groupCount(std::exchange(other.groupCount, 0)),
			m_counterToRehash(other.m_counterToRehash),
			hashFunction(std::move(other.hashFunction)),
			equal(std::move(other.equal)),
			alloc(alloc),
			metadataAlloc(metadataAlloc) { }
	constexpr UnorderedFlatCore(container_rvreference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc)
		requires(allocator_traits::is_always_equal::value && !metadata_allocator_traits::is_always_equal::value) :
			array(std::exchange(other.array, nullptr)),
			size(std::exchange(other.size, 0)),
			m_counterToRehash(other.m_counterToRehash),
			equal(std::move(other.equal)),
			hashFunction(std::move(other.hashFunction)),
			alloc(alloc),
			metadataAlloc(metadataAlloc)
	{
		if (metadataAlloc != other.metadataAlloc) {
			allocateMetadata(other.groupCount);
			swapMetadata(other.metadata, other.metadata + groupCount);
			other.deleteMetadata();
		} else {
			std::swap(metadata, other.metadata);
			std::swap(groupCount, other.groupCount);
		}
	}

	constexpr UnorderedFlatCore(container_rvreference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		hashFunction(std::move(other.hashFunction)),
		equal(std::move(other.equal)),
		alloc(alloc),
		metadataAlloc(metadataAlloc)
	{
		basicRehash(other.bucketCount() - 1);
		insert(other.begin(), other.end());
	}


	constexpr ~UnorderedFlatCore() {
		deleteBuffers();
	}


	constexpr UnorderedFlatCore& operator=(const_container_reference other) {
		if (this == std::addressof(other))
			return *this;

		bool metadataAlreadyCleared = false;

		if constexpr (allocator_traits::propagate_on_container_copy_assignment::value) {
			if (alloc != other.alloc) {
				deleteBuffers();
	
				if constexpr (metadata_allocator_traits::propagate_on_container_copy_assignment::value)
					metadataAlloc = other.metadataAlloc;

				allocateMetadata(other.groupCount);
				array = allocator_traits::allocate(alloc = other.alloc, groupCount * bucketGroupSize());

				metadataAlreadyCleared = true;
			}
		}

		if (!metadataAlreadyCleared) {
			clearArray();
			reallocateOrClearMetadata<metadata_allocator_traits::propagate_on_container_copy_assignment::value>(other.metadataAlloc);
		}

		assignUtilities(other);
		insert(other.begin(), other.end());

		return *this;
	}

	constexpr UnorderedFlatCore& operator=(container_rvreference other)
		noexcept((allocator_traits::propagate_on_container_move_assignment::value || allocator_traits::is_always_equal::value) &&
				 (metadata_allocator_traits::propagate_on_container_move_assignment::value || metadata_allocator_traits::is_always_equal::value))
	{
		if (this == std::addressof(other))
			return *this;

		if constexpr (!allocator_traits::propagate_on_container_move_assignment::value) {
			if (alloc != other.alloc) {
				clearArray();
				reallocateOrClearMetadata<metadata_allocator_traits::propagate_on_container_move_assignment::value>(other.metadataAlloc);

				assignUtilities(other);
				insert(other.begin(), other.end());

				return *this;
			}
		}

		swapUtilities(other);

		if constexpr (!metadata_allocator_traits::propagate_on_container_move_assignment::value) {
			if (metadataAlloc != other.metadataAlloc) {
				deleteArray();
				swapArrayBuffers<allocator_traits::propagate_on_container_move_assignment::value>(other);

				if (groupCount != other.groupCount) {
					deleteMetadata();
					allocateMetadata(other.groupCount);
				}

				swapMetadata(other.metadata, other.metadata + other.groupCount);
				other.deleteMetadata();

				return *this;
			}
		}
 
		swapArrayBuffers<allocator_traits::propagate_on_container_move_assignment::value>(other);
		swapMetadataBuffers<metadata_allocator_traits::propagate_on_container_move_assignment::value>(other);

		return *this;
	}
	constexpr UnorderedFlatCore& operator=(init_list ilist) {
		clear();
		insert(ilist.begin(), ilist.end());

		return *this;
	}


	constexpr void swap(container& other) noexcept {
		swapArrayBuffers<allocator_traits::propagate_on_container_swap::value>(other);
		swapMetadataBuffers<metadata_allocator_traits::propagate_on_container_swap::value>(other);
		swapUtilities(other);
	}


	// Iterator functions

	constexpr iterator begin() noexcept {
		return iterator(metadata, array);
	}
	constexpr const_iterator begin() const noexcept {
		return const_iterator(metadata, array);
	}
	constexpr const_iterator cbegin() const noexcept {
		return const_iterator(metadata, array);
	}
	constexpr iterator end() noexcept {
		return iterator { };
	}
	constexpr const_iterator end() const noexcept {
		return const_iterator { };
	}
	constexpr const_iterator cend() const noexcept {
		return const_iterator { };
	}


	// Capacity modification

	constexpr bool rehash(size_type bucketCount) {
		const auto maxSiz = (groupCount == 0) ? 0 : groupCount * bucketGroupSize() - 1; // Subtract sentinel
		if (maxSiz >= bucketCount) {
			if (m_counterToRehash == 0)
				basicInplaceRehash();

			return false;
		}

		basicRehash(bucketCount);

		return true;
	}
	constexpr bool reserve(size_type count) {
		const auto maxSiz = (groupCount == 0) ? 0 : groupCount * bucketGroupSize() - 1;
		const auto newSiz = static_cast<size_type>(invLFactor * count);

		if (maxSiz >= newSiz) {
			if (m_counterToRehash == 0)
				basicInplaceRehash();
	
			return false;
		}
		
		basicRehash(newSiz);

		return true;
	}


	// Insertion

	template<class Value> constexpr pair_type<iterator, bool> insert(Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		const auto hash = postMixOrHash(pairToKey(value));
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = hashToGroup(hash);

		if constexpr (std::is_same_v<value_type, std::remove_cvref_t<Value>>) {
			const auto it = find<true>(shortHash, groupIndex, pairToKey(value));
			const auto shouldAllocate = it.m_pointer == nullptr;

			return { shouldAllocate ? basicEmplace(hash, shortHash, groupIndex, std::forward<Value>(value)) : it, shouldAllocate };		
		} else {
			value_type v = std::forward<Value>(value);
			const auto it = find<true>(shortHash, groupIndex, pairToKey(v));
			const auto shouldAllocate = it.m_pointer == nullptr;

			return { shouldAllocate ? basicEmplace(hash, shortHash, groupIndex, std::move(v)) : it, shouldAllocate };	
		}
	}

	template <std::input_iterator Iter> constexpr void insert(Iter first, Iter last) {
		if constexpr (std::contiguous_iterator<Iter>)
			reserve(size + std::distance(first, last));

		for (; first != last; first++)
			insert(*first);
	}

	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP_AND_KEY_NON_CONVERTIBLE_TO_ITER {
		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = hashToGroup(hash);

		auto it = find<true>(shortHash, groupIndex, key);

		if (it.m_pointer != nullptr) {
			it.m_pointer->first = std::forward<K>(key);
			it.m_pointer->second = std::forward<V>(value);

			return { it, false };
		} else
			return { basicEmplace(hash, shortHash, groupIndex, std::forward<K>(key), std::forward<V>(value)), true };
	}

	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = hashToGroup(hash);

		const auto it = find<true>(shortHash, groupIndex, key);

		if (it.m_pointer != nullptr)
			return { it, false };
		else
			return { basicEmplace(hash, shortHash, groupIndex, key, mapped_type(std::forward<Args>(args)...)), true };
	}


	template <class H2, class E2> constexpr void merge(UnorderedFlatCore<key_type, mapped_type, H2, E2, allocator_type>& source) noexcept {
		reserve(size + source.size);

		for (auto it = source.begin(); it != iterator { }; it++) {
			const auto& key = pairToKey(*it);
			const auto hash = postMixOrHash(key);
			const auto shortHash = metadata_group::hashToMetadata(hash);
			const auto groupIndex = hashToGroup(hash);

			if (find<false>(shortHash, groupIndex, key) == nullptr) {
				basicEmplace(hash, shortHash, groupIndex, std::move(*it));
				source.erase(it);
			}
		}
	}


	// Erasure

	constexpr iterator erase(iterator pos) {
		assert((pos.m_pointer != nullptr) && "lsd::UnorderedDenseMap::erase(): Invalid iterator was passed to the function!");

		return basicErase(pos);
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		iterator it { first.m_metadata, const_cast<pointer>(first.m_pointer) };

		while (it.m_pointer != nullptr && it != last)
			it = basicErase(it);

		return it;
	}


	constexpr void clear() noexcept {
		clearArray();
		clearMetadata();
	}


	// Lookup

	template <bool GetMetadataPointer, class K> [[nodiscard]] constexpr auto find(size_type shortHash, size_type groupIndex, const K& key) const noexcept {
		for (size_type i = 0; i < groupCount; i++) {
			const auto metadataIt = metadata + (groupIndex = probeFromLast(groupIndex, i));

			if (auto match = metadataIt->match(shortHash); match != 0) {
				const auto it = array + groupIndex * bucketGroupSize();

				do {
					const auto index = std::countr_zero(match);
					match &= match - 1; // match - 1 sets all bits to 1 until the first nonzero bit, which is set to zero, which is used to clear that bit

					const auto curr = it + index;

					if (equal(pairToKey(*curr), key)) {
						if constexpr (GetMetadataPointer)
							return iterator {
								LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + index,
								curr
							};
						else
							return curr;
					}
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash))
				break;
		}

		if constexpr (GetMetadataPointer)
			return iterator { };
		else
			return pointer { };
	}

	template <class K> [[nodiscard]] constexpr true_mapped_type& operator[](K&& key) {
		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto groupIndex = hashToGroup(hash);
		const auto it = (groupCount == 0) ? iterator { } : find<true>(shortHash, groupIndex, key);

		if (it.m_pointer == nullptr) {
			if LSD_UNORDERED_FLAT_IS_SET
				return pairToValue(*basicEmplace(hash, shortHash, groupIndex, std::forward<K>(key)));
			else
				return pairToValue(*basicEmplace(hash, shortHash, groupIndex, std::forward<K>(key), mapped_type()));
		} else
			return pairToValue(*it);
	}

	// Utility and base functions

	static constexpr key_type& pairToKey(value_type& value) noexcept {
		if LSD_UNORDERED_FLAT_IS_SET
			return value;
		else
			return value.first;
	}
	static constexpr true_mapped_type& pairToValue(value_type& value) noexcept {
		if LSD_UNORDERED_FLAT_IS_SET
			return value;
		else
			return value.second;
	}
	static constexpr const key_type& pairToKey(const value_type& value) noexcept {
		if LSD_UNORDERED_FLAT_IS_SET
			return value;
		else
			return value.first;
	}
	static constexpr const true_mapped_type& pairToValue(const value_type& value) noexcept {
		if LSD_UNORDERED_FLAT_IS_SET
			return value;
		else
			return value.second;
	}


	// Hashing utilities

	template <class K> constexpr size_type postMixOrHash(const K& key) const noexcept {
		if constexpr (hashRequiresPostMixValue<hasher>) {
			const auto hash = hashFunction(key);
#if UINTPTR_MAX == UINT64_MAX

#if defined(__SIZEOF_INT128__)
			static constexpr size_type goldenRatioHash = size_type { 0x9E3779B97F4A7C15 };

			const auto hash128 = static_cast<__uint128_t>(hash) * goldenRatioHash;
			return static_cast<size_type>(hash128 >> 64) ^ static_cast<size_type>(hash128);
#elif defined(_MSC_VER) && defined(_M_X64)
			static constexpr std::uint64_t goldenRatioHash = std::uint64_t { 0x9E3779B97F4A7C15 };

			std::uint64_t high { };
			const auto low = _umul128(static_cast<std::uint64_t>(hash), goldenRatioHash, &high);

			return static_cast<size_type>(high ^ low);
#else	
			static constexpr size_type goldenRatioHash = size_type { 0x9E3779B97F4A7C15 };
			static constexpr size_type gHi = goldenRatioHash >> 32;
			static constexpr size_type gLo = goldenRatioHash & 0xFFFFFFFF;

			/**
			 * Cross multiplication algorithm
			 *
			 *             hHi   hLo
			 *           x gHi   gLo
			 *          ------------
			 *             gLo * hLo -> p0
			 *       gLo * hHi       |
			 *       gHi * hLo       |-> middle
			 * gHi * hHi             -> p1
			 *
			 * Principles:
			 * ---
			 * The multiplication of two 32-bit integers can produce a 64-bit integer at max
			 * The addition of two integers can maximally only produce a number one bit larger than itself
			 *
			 * Middle:
			 * ---
			 * The middle part is hHi * gLo + gHi * hLo, which would make for two multiplications
			 * This expression can also be represented by:
			 * (hLo + hHi) * (gLo + gHi) - hLo * gLo - hHi * gHi
			 * where the last two terms are p0 and p1
			 **/

			const size_type hHi = hash >> 32;
			const size_type hLo = hash & size_type { 0xFFFFFFFF };

			const auto p0 = hLo * gLo; // Max. 64 bits
			const auto p1 = hHi * gHi;

			const auto hSum = hLo + hHi; // Max. 33 bits
			const auto gSum = gLo + gHi;

			const auto middleCross = hSum * gSum; // This may overflow to 65 bits, but the bit is not worth recovering
			const auto middle = middleCross - p0 - p1;

			const auto low = p0 + (middle << 32); // This may overflow to 65 bits
			const auto high = p1 + (middle >> 32); // This overflow doesn't matter for mathematical correctness

			return low ^ (high + (low <= p0)); // low <= p0 doesn't guarantee recovery or mathematical correctness of bit lost in low
#endif

#elif UINTPTR_MAX == UINT32_MAX
			std::uint64_t r = static_cast<std::uint64_t>(hash) * 0xE817FB2D;
			return static_cast<size_t>(r) ^ static_cast<size_t>(r >> 32);
#else
#error "lsd::unordered_flat::UnorderedFlatCore::operator(): Container only supports systems with 64-bit or 32-bit word size!"
			return 0;
#endif
		} else return hashFunction(key);
	}

	constexpr size_type hashToGroup(size_type hash) const noexcept {
		static constexpr auto sizeLength = 8 * sizeof(size_type);
		if (groupCount <= 1) [[unlikely]]
			return 0;
		else [[likely]]
			return hash >> (sizeLength - std::countr_zero(groupCount));
	}


	// Getters

	[[nodiscard]] constexpr size_type bucketGroupSize() const noexcept {
		return metadata_group::bucketCount;
	}
	[[nodiscard]] constexpr size_type bucketCount() const noexcept {
		return groupCount * bucketGroupSize();
	}
	[[nodiscard]] constexpr size_type maxBucketCount() const noexcept {
		return maxGroupCount() * bucketGroupSize();
	}

	[[nodiscard]] constexpr size_type groupSize() const noexcept {
		return metadata_group::groupSize;
	}
	[[nodiscard]] constexpr size_type maxGroupCount() const noexcept {
		return metadata_allocator_traits::max_size(metadataAlloc);
	}
	[[nodiscard]] constexpr metadata_allocator_type metadataAllocator() const noexcept {
		return metadataAlloc;
	}


	[[nodiscard]] constexpr double loadFactor() const noexcept {
		return size / static_cast<double>(groupCount * bucketGroupSize());
	}
	[[nodiscard]] constexpr double maxLoadFactor() const noexcept {
		return maxLFactor;
	}


	metadata_pointer metadata { };
	pointer array { };

	size_type size { };
	size_type groupCount { }; // Always a power of two

private:
	size_type m_counterToRehash { };

public:
	[[no_unique_address]] hasher hashFunction { };
	[[no_unique_address]] key_equal equal { };

	[[no_unique_address]] allocator_type alloc { };
	[[no_unique_address]] metadata_allocator_type metadataAlloc { };


private:
	// Utility functions

	constexpr size_type prober(size_type base, size_type index) const noexcept {
		return (base + (index * (index + 1) >> 1)) & (groupCount - 1);
	}
	constexpr size_type probeFromLast(size_type last, size_type index) const noexcept {
		return (last + index) & (groupCount - 1);
	}


	// Memory management

	constexpr void deleteBuffers() {
		deleteArray();
		deleteMetadata();
	}

	constexpr void clearArray() {
		if constexpr (!std::is_trivially_destructible_v<value_type>)
			for (auto it = begin(); it.m_pointer != nullptr; it++)
				allocator_traits::destroy(alloc, it.m_pointer);

		size = 0;
	}
	constexpr void deleteArray() {
		if (!array)
			return;

		clearArray();
		allocator_traits::deallocate(alloc, array, groupCount * bucketGroupSize());
		array = nullptr;
	}


	constexpr void allocateMetadata(size_type count) {
		groupCount = count;
		metadata = metadata_allocator_traits::allocate(metadataAlloc, groupCount);
		initMetadata();
	}
	constexpr void deleteMetadata() {
		if (!metadata)
			return;
		
		metadata_allocator_traits::deallocate(metadataAlloc, metadata, groupCount);
		metadata = nullptr;
		groupCount = 0;
	}

	template <bool Propagate>
	constexpr void reallocateOrClearMetadata(const metadata_allocator_type& metadataAlloc) {
		if constexpr (Propagate) {
			if (metadataAlloc != metadataAlloc) {
				deleteMetadata();
				this->metadataAlloc = metadataAlloc;
				allocateMetadata(groupCount);

				return;
			}
		}
		
		clearMetadata();
	}

	constexpr void initMetadata() {
		const auto* const end = metadata + groupCount;
		for (auto metadataIt = metadata; metadataIt != end; metadataIt++)
			metadata_allocator_traits::construct(metadataAlloc, metadataIt);

		(metadata + (groupCount - 1))->insertSentinel();
	}

	constexpr void swapMetadata(metadata_group* otherIt, const metadata_group* const end) {
		for (auto it = metadata; otherIt != end; it++, otherIt++)
			std::swap(*it, *otherIt);
	}

	constexpr void clearMetadata() {
		if (groupCount == 0)
			return;
	
		if consteval {
			const auto* const end = metadata + groupCount - 1;
			for (auto it = metadata; it != end; it++)
				*it = metadata_group { };
		} else {
			std::memset(metadata, 0, (groupCount - 1) * sizeof(metadata_group));
		}
		
		(metadata + groupCount - 1)->clearAndSetSentinel();
	}

	template <bool SwapAlloc>
	constexpr void swapArrayBuffers(container& other) noexcept {
		std::swap(array, other.array);

		std::swap(size, other.size);
		std::swap(m_counterToRehash, other.m_counterToRehash);

		if constexpr (SwapAlloc)
			std::swap(alloc, other.alloc);
	}
	template <bool SwapAlloc>
	constexpr void swapMetadataBuffers(container& other) noexcept {
		std::swap(metadata, other.metadata);
		std::swap(groupCount, other.groupCount);

		if constexpr (SwapAlloc)
			std::swap(metadataAlloc, other.metadataAlloc);
	}
	constexpr void swapUtilities(container& other) noexcept {
		if constexpr (!std::is_empty_v<hasher>)
			std::swap(hashFunction, other.hashFunction);
		if constexpr (!std::is_empty_v<key_equal>)
			std::swap(equal, other.equal);
	}

	constexpr void assignUtilities(const_container_reference other) noexcept {
		hashFunction = other.hashFunction;
		equal = other.equal;
	}


	// Insertion and manipulation base functions

	template <bool GetMetadataPointer>
	constexpr auto insertShortHashAndGetIterator(size_type shortHash, size_type groupIndex) noexcept {
		auto metadataIt = metadata + groupIndex;
		auto occupied = metadataIt->matchOccupied();

		for (size_type i = 1; occupied == metadata_group::full && i < groupCount; i++) {
			metadataIt->markOverflow(shortHash);
			metadataIt = metadata + (groupIndex = probeFromLast(groupIndex, i));

			occupied = metadataIt->matchOccupied();
		}

		// Group found with empty elements still available
		const auto bucketIndex = std::countr_one(occupied);
		metadataIt->insert(bucketIndex, shortHash);

		if constexpr (GetMetadataPointer)
			return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + bucketIndex, array + groupIndex * bucketGroupSize() + bucketIndex };
		else
			return array + groupIndex * bucketGroupSize() + bucketIndex;
	}

	template <class... Args>
	constexpr iterator basicEmplace(size_type hash, size_type shortHash, size_type groupIndex, Args&&... args) {
		if (reserve(size + 1))
			groupIndex = hashToGroup(hash);

		const auto it = insertShortHashAndGetIterator<true>(shortHash, groupIndex);
		allocator_traits::construct(alloc, it.m_pointer, std::forward<Args>(args)...);
		++size;

		return it;
	}


	constexpr iterator basicErase(iterator pos) {
		auto it = pos++;

		const auto diff = it.groupIndex();
		auto metadataFromIt = it.metadataArray(diff);
		if (metadata_group::isSentinel(metadataFromIt, diff))
			throw std::out_of_range("lsd::UnorderedDenseMap::erase(): can't erase sentinel!");

		m_counterToRehash = (m_counterToRehash == 0) ? 0 : m_counterToRehash - metadata_group::maybeCausedOverflow(metadataFromIt, diff);
		metadata_group::erase(metadataFromIt, diff);

		allocator_traits::destroy(alloc, it.m_pointer);
		--size;

		return pos;
	}


	constexpr void basicRehash(size_type bucketCount) {
		if (bucketCount > maxBucketCount())
			throw std::length_error("lsd::UnorderedFlatCore::basicRehash(): Requested size larger than the maximum size of the container!");

		// bucketCount represents the minimum amount of buckets to allocate. Therefore, we round it up
		// to the next multiple of bucketGroupSize(). To guarantee successful rounding, we add bucketGroupSize() - 1
		// To the original bucketCount. To compensate for the sentinel, we must add 1, cancelling out the
		// subtraction. This number is then divided by bucketGroupSize() to recieve the number of groups needed,
		// which is the round up to the next largest power of two

		const auto newGroupCount = std::bit_ceil((bucketCount + bucketGroupSize()) / bucketGroupSize());
		const auto oldGroupCount = std::exchange(groupCount, newGroupCount);
		const auto arraySize = groupCount * bucketGroupSize();

		auto oldMetadata = std::exchange(metadata, metadata_allocator_traits::allocate(metadataAlloc, groupCount));
		auto oldArray = std::exchange(array, allocator_traits::allocate(alloc, arraySize));

		// Setup metadata
		initMetadata();
		m_counterToRehash = arraySize * maxLFactor; // Number of usable metadata slots


		// If oldArray was valid, we need to transfer elements and deallocate the memory
		// We assume that the metadata will also be valid

		if (!oldArray)
			return;

		// If the old array wasn't empty, we move over the old elements
		if (size != 0) {
			for (auto it = iterator(oldMetadata, oldArray); it.m_pointer != nullptr; it++) {
				const auto hash = postMixOrHash(pairToKey(*it));

				allocator_traits::construct(alloc, insertShortHashAndGetIterator<false>(metadata_group::hashToMetadata(hash), hashToGroup(hash)), std::move(*it));
				allocator_traits::destroy(alloc, it.m_pointer);
			}
		}

		allocator_traits::deallocate(alloc, oldArray, oldGroupCount * bucketGroupSize());
		metadata_allocator_traits::deallocate(metadataAlloc, oldMetadata, oldGroupCount);
	}
	constexpr void basicInplaceRehash() {
		// Reset all overflow bytes
		for (auto metadataIt = metadata, end = metadata + groupCount; metadataIt != end; metadataIt++)
			metadataIt->clearOverflow();

		for (auto it = begin(); it.m_pointer != nullptr; it++) {
			const auto hash = postMixOrHash(pairToKey(*it));
			const auto shortHash = metadata_group::hashToMetadata(hash);

			const size_type groupIndex = hashToGroup(hash);
			auto metadataIt = metadata + groupIndex;

			const auto diff = it.groupIndex();
			auto metadataFromIt = it.metadataArray(diff);
			if (metadataFromIt != metadataIt->data()) { // Attempts to find a new location for values not in the original bucket
				metadata_group::erase(metadataFromIt, diff);

				if (auto newIt = insertShortHashAndGetIterator<false>(shortHash, groupIndex); newIt != it.m_pointer) {
					allocator_traits::construct(alloc, newIt, std::move(*it));
					allocator_traits::destroy(alloc, it.m_pointer);
				}
			}
		}

		m_counterToRehash = (maxLFactor * groupCount) * bucketGroupSize();
	}
};

} // namespace unordered_flat

} // namespace lsd
