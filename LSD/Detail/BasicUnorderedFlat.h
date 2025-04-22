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

#include "CoreUtility.h"
#include "../Iterators.h"
#include "../Vector.h"
#include "../Array.h"
#include "../Hash.h"

#include <cstdint>
#include <utility>
#include <limits>
#include <bit>
#include <functional>
#include <initializer_list>
#include <type_traits>


#define LSD_UNORDERED_FLAT_TO_BYTE(metadata) reinterpret_cast<metadata_group::byte_pointer>(metadata)
#define LSD_UNORDERED_FLAT_IS_SET constexpr (std::is_void_v<mapped_type>)
#define LSD_UNORDERED_FLAT_REQUIRES_MAP requires(!std::is_void_v<mapped_type>)


namespace lsd {

namespace unordered_flat {

template <class, class, class, class, class> class BasicUnorderedFlat;

namespace detail {

/**
 * Huuuge thank you to https://github.com/joaquintides for explaining the super awesome algorithm for the hash matching
 * I could NOT figure this out until you explained the algorithm behind this step by step, really appreciate it a lot
 *
 * If Mr. Muñoz happens to stumble across this, again, thank you very much, you're an awesome guy, and please keep up your contributions for the open source community!
 */
class MetadataGroup {
public:
	using size_type = std::size_t;
	
	using word_type = std::uint64_t;
	using word_pointer = const word_type*;

	using byte_type = std::uint8_t;
	using byte_pointer = const byte_type*;

	static constexpr size_type available = 0;
	static constexpr size_type sentinel = 1;
	static constexpr size_type sentinelIndex = 14;

	static constexpr size_type bucketSize = 15;
	static constexpr size_type groupSize = 16;

	static constexpr word_type empty = 0x0000;
	static constexpr std::uint32_t full = 0x7FFF;


	constexpr void insert(size_type index, size_type hash) noexcept {
		basicInsert(index, hashToMetadata(hash));
	}
	constexpr void erase(size_type index) noexcept {
		basicInsert(index, available);
	}
	constexpr void clear() noexcept {
		m_metadata[0] = 0;
		m_metadata[1] = 0;
	}
	constexpr bool occupied(size_type index) const noexcept {
		return ((m_metadata[0] | m_metadata[1]) & (UINT64_C(0x0001000100010001) << index)) != 0;
	}

	constexpr void insertSentinel() noexcept {
		basicInsert(14, sentinel); // Inserted at the 15th place, since the OFW byte can't be used
	}
	constexpr void clearExceptSentinel() noexcept {
		m_metadata[0] = 1 << 14, m_metadata[1] = 0;
	}
	constexpr bool isSentinel(size_type index) const noexcept {
		return index == 14 && (m_metadata[0] & UINT64_C(0x4000400040004000)) == 0x4000 && (m_metadata[1] & UINT64_C(0x4000400040004000)) == 0;
	}

	constexpr void markOverflow(size_type hash) noexcept {
		reinterpret_cast<uint16_t*>(m_metadata.data())[hash & 7] |= 0x8000;
	}
	constexpr void clearOverflow() noexcept {
		basicInsert(15, available);
	}
	constexpr bool overflowed(size_type hash) const noexcept {
		return reinterpret_cast<const uint16_t*>(m_metadata.data())[hash & 7] & 0x8000;
	}

	constexpr std::uint32_t match(size_type hash) const noexcept {
		static constexpr lsd::Array<word_type, 16> multiplexBitPattern = {
			UINT64_C(0x0000000000000000), UINT64_C(0x000000000000FFFF), UINT64_C(0x00000000FFFF0000), UINT64_C(0x00000000FFFFFFFF),
			UINT64_C(0x0000FFFF00000000), UINT64_C(0x0000FFFF0000FFFF), UINT64_C(0x0000FFFFFFFF0000), UINT64_C(0x0000FFFFFFFFFFFF),
			UINT64_C(0xFFFF000000000000), UINT64_C(0xFFFF00000000FFFF), UINT64_C(0xFFFF0000FFFF0000), UINT64_C(0xFFFF0000FFFFFFFF),
			UINT64_C(0xFFFFFFFF00000000), UINT64_C(0xFFFFFFFF0000FFFF), UINT64_C(0xFFFFFFFFFFFF0000), UINT64_C(0xFFFFFFFFFFFFFFFF)
		};

		word_type matchAndFold = ~((m_metadata[0] ^ multiplexBitPattern[hash & 0xF]) | (m_metadata[1] ^ multiplexBitPattern[hash >> 4])); /// @todo Won't work on little endian
		matchAndFold &= matchAndFold >> 32; // Fold again

		return static_cast<std::uint32_t>(matchAndFold & matchAndFold >> 16) & full; // Fold one last time and discard and truncate
	}
	constexpr std::uint32_t matchOccupied() const noexcept {
		word_type fold = m_metadata[0] | m_metadata[1];
		fold |= (fold >> 32);

		return static_cast<std::uint32_t>(fold | (fold >> 16)) & full;
	}


	// Utility functions

	static constexpr size_type hashToMetadata(size_type hash) noexcept {
		// Table has to start at 8, due to the modulo of the original hash having to be equal to the mod of the reduced hash, whilst 0 and 1 are reserved
		static constexpr lsd::Array<byte_type, 256> validReducedHashes {
			0x08, 0x09, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
			0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
			0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
			0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
			0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
			0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
			0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
			0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
		};

		return validReducedHashes[hash & 0xFF];
	}


	// Static versions of some functions

	static constexpr bool occupied(word_pointer metadata, size_type index) noexcept {
		return ((metadata[0] | metadata[1]) & (UINT64_C(0x0001000100010001) << index)) != 0;
	}

	static constexpr std::uint32_t matchOccupied(word_pointer metadata) noexcept {
		word_type fold = metadata[0] | metadata[1];
		fold |= (fold >> 32);

		return static_cast<std::uint32_t>(fold | (fold >> 16)) & full;
	}

	static constexpr bool isSentinel(word_pointer metadata, size_type index) noexcept {
		return index == 14 && (metadata[0] & UINT64_C(0x4000400040004000)) == 0x4000 && (metadata[1] & UINT64_C(0x4000400040004000)) == 0;
	}

private:
	constexpr void insertWord(word_type& word, size_type pos, size_type metadata) noexcept {
		static constexpr lsd::Array<word_type, 16> interleavedBitPattern = {
			UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001), UINT64_C(0x0000000000010000), UINT64_C(0x0000000000010001),
			UINT64_C(0x0000000100000000), UINT64_C(0x0000000100000001), UINT64_C(0x0000000100010000), UINT64_C(0x0000000100010001),
			UINT64_C(0x0001000000000000), UINT64_C(0x0001000000000001), UINT64_C(0x0001000000010000), UINT64_C(0x0001000000010001),
			UINT64_C(0x0001000100000000), UINT64_C(0x0001000100000001), UINT64_C(0x0001000100010000), UINT64_C(0x0001000100010001)
		};

		word &= ~(interleavedBitPattern[15] << pos);
		word |= interleavedBitPattern[metadata] << pos;
	}
	constexpr void basicInsert(size_type pos, size_type metadata) noexcept {
		insertWord(m_metadata[0], pos, metadata & 15);
		insertWord(m_metadata[1], pos, metadata >> 4);
	}

	
	alignas(16) lsd::Array<word_type, 2> m_metadata { 0, 0 }; // Enfore 16 byte alignment even on 32 bit systems
};

} // namespace detail


template <class Ty> class Iterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::contiguous_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using pointer_const = const_value*;
	using reference = value_type&;

	using metadata_group = detail::MetadataGroup;
	using group_pointer = const metadata_group*;
	using word_pointer = metadata_group::word_pointer;
	using metadata_pointer = metadata_group::byte_pointer; // Won't be modified anyways

	using container = Iterator;
	using container_rvreference = Iterator&&;
	using container_reference = container&;
	using const_container_reference = const container&;

	
	constexpr Iterator() noexcept = default;
	constexpr Iterator(const_container_reference) noexcept = default;
	constexpr Iterator(container_rvreference) noexcept = default;

	constexpr container_reference operator=(const_container_reference) noexcept = default;
	constexpr container_reference operator=(container_rvreference) noexcept = default;

	constexpr container_reference operator++() noexcept {
		auto diff = reinterpret_cast<uintptr_t>(m_metadata) & metadata_group::bucketSize;
		m_metadata -= diff;

		// Shift metadata to the next position
		auto occupied = (metadata_group::matchOccupied(reinterpret_cast<word_pointer>(m_metadata)) >> (diff + 1)) << (diff + 1);

		while (occupied == 0) {
			m_metadata += metadata_group::groupSize;
			m_pointer += metadata_group::bucketSize;
			
			occupied = metadata_group::matchOccupied(reinterpret_cast<word_pointer>(m_metadata));
		}

		auto emptyCount = std::countr_zero(occupied);
		if (metadata_group::isSentinel(reinterpret_cast<word_pointer>(m_metadata), emptyCount))
			m_pointer = nullptr;
		else {
			m_metadata += emptyCount;
			m_pointer -= diff;
			m_pointer += emptyCount;
		}

		return *this;
	}
	constexpr container operator++(int) noexcept { 
		container tmp = *this; 
		++(*this); 
		return tmp; 
	}

	constexpr reference operator*() const {
		return *m_pointer;
	}
	constexpr pointer operator->() const noexcept {
		return m_pointer;
	}
	constexpr pointer get() const noexcept {
		return m_pointer;
	}

	constexpr operator Iterator<const_value>() const noexcept {
		return Iterator(m_metadata, m_pointer);
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
	metadata_pointer m_metadata = nullptr;
	pointer m_pointer = nullptr;

	constexpr Iterator(metadata_pointer metadata, pointer pointer) noexcept :
		m_metadata(metadata), m_pointer(pointer) { }
	constexpr Iterator(group_pointer metadata, pointer pointer) noexcept :
		m_metadata(reinterpret_cast<metadata_pointer>(metadata)), m_pointer(pointer)
	{
		if (!m_pointer) return;

		// This function is only called during the creation of begin iterators, hence it always starts at 0
		if (metadata->occupied(0)) return;

		auto occupied = metadata->matchOccupied();
		while (occupied == 0) { // Skip empty groups, since a non-empty or sentinel group has to come eventually
			m_metadata += metadata_group::groupSize;
			m_pointer += metadata_group::bucketSize;

			occupied = (++metadata)->matchOccupied();
		}

		auto emptyCount = std::countr_zero(occupied);
		if (metadata->isSentinel(emptyCount))
			m_pointer = nullptr;
		else {
			m_metadata += emptyCount;
			m_pointer += emptyCount;
		}
	}
	
	template <class, class, class, class, class> friend class BasicUnorderedFlat;
};


template <
	class Key,
	class Ty, // When Ty is void, treat as a set
	class Hash,
	class Equal,
	class Alloc
> class BasicUnorderedFlat {
public:
	// Type aliases and constants
	
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	template <class First, class Second> using pair_type = std::pair<First, Second>;

	static constexpr float maxLFactor = 0.875;
	static constexpr size_type bucketMinCount = 1;

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

public:
	// Construction and assignment

	constexpr BasicUnorderedFlat() {
		basicRehash(1);
	}

	explicit constexpr BasicUnorderedFlat(
		size_type bucketCount, 
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const metadata_allocator_type& metadataAlloc = metadata_allocator_type()
	) : m_hasher(hash), m_equal(keyEqual), m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		basicRehash(std::max(size_type { 1 }, bucketCount));
	}
	constexpr BasicUnorderedFlat(
		size_type bucketCount,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc
	) : m_alloc(alloc), m_metadataAlloc(alloc) {
		basicRehash(std::max(size_type { 1 }, bucketCount));
	}
	constexpr BasicUnorderedFlat(
		size_type bucketCount,
		const hasher& hasher,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc
	) : m_hasher(hasher), m_alloc(alloc), m_metadataAlloc(alloc) {
		basicRehash(std::max(size_type { 1 }, bucketCount));
	}
	constexpr BasicUnorderedFlat(const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		m_alloc(alloc), m_metadataAlloc(alloc) {
		basicRehash(1);
	}

	template <ContinuousIteratorType It> constexpr BasicUnorderedFlat(
		It first, It last, 
		size_type bucketCount = bucketMinCount,
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const metadata_allocator_type& metadataAlloc = metadata_allocator_type()
	) : m_hasher(hash), m_equal(keyEqual), m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(first, last);
	}
	template <ContinuousIteratorType It> constexpr BasicUnorderedFlat(
		It first, It last,
		size_type bucketCount,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc
	) : m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(first, last);
	}
	template <ContinuousIteratorType It> constexpr BasicUnorderedFlat(
		It first, It last, 
		size_type bucketCount,
		const hasher& hash,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc
	) : m_hasher(hash), m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(first, last);
	}
		
	constexpr BasicUnorderedFlat(const_container_reference other) {
		insert(other.begin(), other.end());
	}
	constexpr BasicUnorderedFlat(const_container_reference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(other.begin(), other.end());
	}
	constexpr BasicUnorderedFlat(container_rvreference other) noexcept :
		m_metadata(std::exchange(other.m_metadata, nullptr)),
		m_array(std::exchange(other.m_array, nullptr)),
		m_size(other.m_size),
		m_bucketCount(other.m_bucketCount),
		m_loadFactor(other.m_loadFactor),
		m_alloc(std::move(other.m_alloc)),
		m_metadataAlloc(std::move(other.m_metadataAlloc)),
		m_hasher(std::move(other.m_hasher)),
		m_equal(std::move(other.m_equal)) { }
	constexpr BasicUnorderedFlat(container_rvreference other, const allocator_type& alloc, const metadata_allocator_type& metadataAlloc) :
		m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		if constexpr (lsd::detail::allocatorPropagationNecessary(other.m_alloc, alloc))
			moveInsert(other.begin(), other.end());
		else {
			m_metadata = std::exchange(other.m_metadata, nullptr);
			m_array = std::exchange(other.m_array, nullptr);
			m_size = other.m_size;
			m_bucketCount = other.m_bucketCount;
			m_loadFactor = other.m_loadFactor;
			m_hasher = std::move(other.m_hasher);
			m_equal = std::move(other.m_equal);
		}
	}

	constexpr BasicUnorderedFlat(
		init_list ilist, 
		size_type bucketCount = bucketMinCount,
		const hasher& hash = hasher(), 
		const key_equal& keyEqual = key_equal(), 
		const allocator_type& alloc = allocator_type(),
		const metadata_allocator_type& metadataAlloc = metadata_allocator_type()
	) : m_hasher(hash), m_equal(keyEqual), m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(ilist.begin(), ilist.end());
	}
	constexpr BasicUnorderedFlat(
		init_list ilist,
		size_type bucketCount,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc = metadata_allocator_type()
	) : m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(ilist.begin(), ilist.end());
	} 
	constexpr BasicUnorderedFlat(
		init_list ilist,
		size_type bucketCount,
		const hasher& hash,
		const allocator_type& alloc,
		const metadata_allocator_type& metadataAlloc = metadata_allocator_type()
	) : m_hasher(hash), m_alloc(alloc), m_metadataAlloc(metadataAlloc) {
		insert(ilist.begin(), ilist.end());
	}
	
	constexpr ~BasicUnorderedFlat() {
		if (m_array) {
			for (auto it = begin(); it.m_pointer != nullptr; it++)
				allocator_traits::destroy(m_alloc, it.m_pointer);

			metadata_allocator_traits::deallocate(m_metadataAlloc, m_metadata, m_bucketCount);
			allocator_traits::deallocate(m_alloc, m_array, m_bucketCount * 15);

			m_bucketCount = 0;
			m_size = 0;
		}
	}


	constexpr BasicUnorderedFlat& operator=(const_container_reference other) noexcept {
		clear();
		insert(other.begin(), other.end());

		return *this;
	}
	constexpr BasicUnorderedFlat& operator=(container_rvreference other) noexcept {
		if constexpr (lsd::detail::allocatorPropagationNecessary(other.m_alloc, m_alloc)) {
			clear();
			moveInsert(other.begin(), other.end());
		} else swap(other);

		return *this;
	}
	constexpr BasicUnorderedFlat& operator=(init_list ilist) {
		clear();
		insert(ilist.begin(), ilist.end());

		return *this;
	}

	constexpr void swap(container& other) noexcept {
		std::swap(m_array, other.m_array);
		std::swap(m_metadata, other.m_metadata);

		std::swap(m_size, other.m_size);
		std::swap(m_bucketCount, other.m_bucketCount);
		std::swap(m_loadFactor, other.m_loadFactor);

		if constexpr (allocator_traits::propagate_on_container_swap::value == true)
			std::swap(m_alloc, other.m_alloc);
		if constexpr (metadata_allocator_traits::propagate_on_container_swap::value == true)
			std::swap(m_metadataAlloc, other.m_metadataAlloc);

		if constexpr (!std::is_empty_v<hasher>) std::swap(m_hasher, other.m_hasher);
		if constexpr (!std::is_empty_v<key_equal>) std::swap(m_equal, other.m_equal);
	}


	// Iterator functions

	constexpr iterator begin() noexcept {
		return iterator(m_metadata, m_array);
	}
	constexpr const_iterator begin() const noexcept {
		return const_iterator(m_metadata, m_array);
	}
	constexpr const_iterator cbegin() const noexcept {
		return const_iterator(m_metadata, m_array);
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

	[[nodiscard]] constexpr reference front() noexcept {
		return *m_array;
	}
	[[nodiscard]] constexpr const_reference front() const noexcept {
		return *m_array;
	}
	[[nodiscard]] constexpr reference back() noexcept {
		return *(m_array + m_size - 1);
	}
	[[nodiscard]] constexpr const_reference back() const noexcept {
		return *(m_array + m_size - 1);
	}


	// Capacity modification

	constexpr bool rehash(size_type bucketCount) {
		if (m_size / static_cast<float>(m_bucketCount * 15) > maxLFactor) {
			auto count = std::max(m_bucketCount, size_type { 1 });
			while (count *= 2 < bucketCount)
				; // Guaranteed not to overflow due to above checks

			basicRehash(count);

			return true;
		}

		return false;
	}
	constexpr bool reserve(size_type count) {
		if (count / static_cast<float>(m_bucketCount * 15) > maxLFactor) {
			auto bucketCount = std::max(m_bucketCount, size_type { 1 });
			while ((bucketCount *= 2) * 15 * maxLFactor < count)
				; // Guaranteed not to overflow due to above checks

			basicRehash(bucketCount);

			return true;
		}

		return false;
	}


	// Insertion

	constexpr pair_type<iterator, bool> insert(const_reference value) {
		const auto hash = valueToHash(value);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		if LSD_UNORDERED_FLAT_IS_SET {
			auto it = find(hash, shortHash, bucketIndex, value);
			
			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, value), true };
		} else {
			auto it = find(hash, shortHash, bucketIndex, value.first);
			
			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, value), true };
		}
	}
	template <class Value> constexpr pair_type<iterator, bool> insert(Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		const auto hash = valueToHash(value);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		if LSD_UNORDERED_FLAT_IS_SET {
			auto it = find(hash, shortHash, bucketIndex, value);

			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, std::forward<Value>(value)), true };
		} else {
			value_type v = std::forward<Value>(value);
			auto it = find(hash, shortHash, bucketIndex, v.first);

			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, std::move(v)), true };
		}
	}
	constexpr iterator insert(const_iterator hint, const_reference value) {
		const auto hash = valueToHash(value);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		if LSD_UNORDERED_FLAT_IS_SET {
			auto it = find(hash, shortHash, bucketIndex, value);
			
			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, value), true };
		} else {
			auto it = find(hash, shortHash, bucketIndex, value.first);
			
			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, value), true };
		}
	}
	template <class Value> constexpr iterator insert(const_iterator hint, Value&& value) requires(std::is_constructible_v<value_type, Value&&>) {
		const auto hash = valueToHash(value);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		if LSD_UNORDERED_FLAT_IS_SET {
			auto it = find(hash, shortHash, bucketIndex, value);

			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, std::forward<Value>(value)), true };
		} else {
			value_type v = std::forward<Value>(value);
			auto it = find(hash, shortHash, bucketIndex, v.first);

			if (it.m_pointer != nullptr) return { it, false };
			else return { basicInsert(hash, shortHash, bucketIndex, std::move(v)), true };
		}
	}
	template <IteratorType It> constexpr void insert(It first, It last) {
		for (; first != last; first++) insert(*first);
	}
	constexpr void insert(init_list ilist) {
		insert(ilist.begin(), ilist.end());
	}


	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const key_type& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) {
			*it = value_type(key, std::forward<V>(value));

			return { it, false };
		} else return { basicInsert(hash, shortHash, bucketIndex, value_type(key, std::forward<V>(value))), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) {
			*it = value_type(std::forward<K>(key), std::forward<V>(value));

			return { it, false };
		} else return { basicInsert(hash, shortHash, bucketIndex, value_type(std::forward<K>(key), std::forward<V>(value))), true };
	}
	template <class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, const key_type& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) {
			*it = value_type(key, std::forward<V>(value));

			return { it, false };
		} else return { basicInsert(hash, shortHash, bucketIndex, std::move(value_type(key, std::forward<V>(value)))), true };
	}
	template <class K, class V>
	constexpr pair_type<iterator, bool> insertOrAssign(const_iterator hint, K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) {
			*it = value_type(std::forward<K>(key), std::forward<V>(value));

			return { it, false };
		} else return { basicInsert(hash, shortHash, bucketIndex, value_type(std::forward<K>(key), std::forward<V>(value))), true };
	}

	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const key_type& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(key, std::forward<V>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(std::forward<K>(key), std::forward<V>(value));
	}
	template <class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, const key_type& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		return insertOrAssign(hint, key, std::forward<V>(value));
	}
	template <class K, class V>
	[[deprecated]] constexpr pair_type<iterator, bool> insert_or_assign(const_iterator hint, K&& key, V&& value) LSD_UNORDERED_FLAT_REQUIRES_MAP {
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
	constexpr pair_type<iterator, bool> tryEmplace(const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) return { it, false };
		else return { basicInsert(hash, shortHash, bucketIndex, value_type(key, mapped_type(std::forward<Args>(args)...))), true };
	}
	template <class K, class... Args>
	constexpr pair_type<iterator, bool> tryEmplace(K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) return { it, false };
		else return { basicInsert(hash, shortHash, bucketIndex, value_type(key, mapped_type(std::forward<Args>(args)...))), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) return { it, false };
		else return { basicInsert(hash, shortHash, bucketIndex, value_type(key, mapped_type(std::forward<Args>(args)...))), true };
	}
	template <class K, class... Args>
	constexpr iterator tryEmplace(const_iterator hint, K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		const auto hash = valueToHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hint.m_metadata - m_metadata;

		auto it = find(hash, shortHash, bucketIndex, key);

		if (it.m_pointer != nullptr) return { it, false };
		else return { basicInsert(hash, shortHash, bucketIndex, value_type(key, mapped_type(std::forward<Args>(args)...))), true };
	}

	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		tryEmplace(key, args...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		tryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
	}
	template <class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, const key_type& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		tryEmplace(hint, key, args...);
	}
	template <class K, class... Args>
	[[deprecated]] constexpr iterator try_emplace(const_iterator hint, K&& key, Args&&... args) LSD_UNORDERED_FLAT_REQUIRES_MAP {
		tryEmplace(hint, std::forward<K>(key), std::forward<Args>(args)...);
	}


	template <class H2, class E2> constexpr void merge(BasicUnorderedFlat<key_type, mapped_type, H2, E2, allocator_type>& source) noexcept {
		insert(source.begin(), source.end());
	}
	template <class H2, class E2> constexpr void merge(BasicUnorderedFlat<key_type, mapped_type, H2, E2, allocator_type>&& source) noexcept {
		insert(source.begin(), source.end());
	}


	// Erasure

	constexpr iterator erase(iterator pos) {
		assert((pos.m_pointer != nullptr) && "lsd::UnorderedDenseMap::erase(): Invalid iterator was passed to the function!");

		auto diff = reinterpret_cast<uintptr_t>(pos.m_metadata) & metadata_group::bucketSize;
		auto metadata = reinterpret_cast<metadata_pointer>(const_cast<metadata_group::byte_type*>(pos.m_metadata) - diff);

		assert((!metadata->isSentinel(diff)) && "lsd::UnorderedDenseMap::erase(): can't erase sentinel!");

		metadata->erase(diff);
		allocator_traits::destroy(m_alloc, pos.m_pointer);
		--m_size;

		if (--m_loadFactor == 0) basicInplaceRehash();

		return pos;
	}
	constexpr iterator erase(const_iterator pos) {
		return erase(iterator { pos.m_metadata, const_cast<pointer>(pos.m_pointer) });
	}
	constexpr iterator erase(const_iterator first, const_iterator last) {
		size_type eraseCount = 0;

		iterator it { first.m_metadata, const_cast<pointer>(first.m_pointer) };
		iterator origIt = it;

		while (it.m_pointer != nullptr && it != last) {
			// Cast metadata byte to the metadata structure
			auto diff = reinterpret_cast<uintptr_t>(it.m_metadata) & metadata_group::bucketSize;
			auto metadata = reinterpret_cast<metadata_pointer>(const_cast<metadata_group::byte_type*>(it.m_metadata) - diff);
			if (metadata->isSentinel(diff)) throw std::out_of_range("lsd::UnorderedDenseMap::erase(): can't erase sentinel!");

			metadata->erase(diff);
			allocator_traits::destroy(m_alloc, it.m_pointer);

			++it, ++eraseCount;
		}

		m_size -= eraseCount;
		if (m_loadFactor < eraseCount || (m_loadFactor -= eraseCount) == 0) basicInplaceRehash();

		return origIt;
	}
	constexpr size_type erase(const key_type& key) {
		if (auto it = find(key); it.m_pointer != nullptr) {
			erase(it);
			return 1;
		}

		return 0;
	}
	template <class K> constexpr size_type erase(K&& key) {
		if (auto it = find(std::forward<K>(key)); it.m_pointer != nullptr) {
			erase(it);
			return 1;
		}

		return 0;
	}

	constexpr value_type extract(const_iterator pos) {
		assert((pos.m_pointer != nullptr) && "lsd::BasicUnorderedFlat::extract(): Invalid iterator was passed to the function!");

		value_type res(std::move(*const_cast<value_type*>(pos.get())));
		erase(pos);
		return res;
	}
	constexpr value_type extract(const key_type& key) {
		if (auto it = find(key); it.m_pointer != nullptr) return extract(it);
		return value_type();
	} 
	template <class K> constexpr value_type extract(K&& key) {
		if (auto it = find(std::forward<K>(key)); it.m_pointer != nullptr) return extract(it);
		return value_type();
	}

	constexpr void clear() noexcept {
		for (auto it = begin(); it.m_pointer != nullptr; it++)
			allocator_traits::destroy(m_alloc, it.m_pointer);
		
		const auto metadataEnd = m_metadata + (m_bucketCount - 1);
		auto metadata = m_metadata;

		while (metadata != metadataEnd) {
			metadata->clear();
			++metadata;
		}
		metadata->clearExceptSentinel();

		m_size = 0;
	}


	// Lookup

	template <class K> [[nodiscard]] constexpr iterator find(const K& key) noexcept {
		if (m_bucketCount == 0) return iterator { };

		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		auto bucketIndex = hashToBucket(hash);

		for (size_type i = 0; i < m_bucketCount; i++) {
			auto location = prober(bucketIndex, i);
			const auto* const metadataIt = m_metadata + location;

			if (auto match = metadataIt->match(shortHash); match != 0) {
				auto* const it = m_array + location * 15;

				do {
					auto toNext = std::countr_zero(match);
					match &= match - 1;

					if LSD_UNORDERED_FLAT_IS_SET {
						if (auto candidate = it + toNext; m_equal(*candidate, key))
							return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
					} else if (auto candidate = it + toNext; m_equal(candidate->first, key))
						return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash)) return iterator { };
		}

		return iterator { };
	}
	template <class K> [[nodiscard]] constexpr const_iterator find(const K& key) const noexcept {
		if (m_bucketCount == 0) return const_iterator { };

		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		auto bucketIndex = hashToBucket(hash);

		for (size_type i = 0; i < m_bucketCount; i++) {
			auto location = prober(bucketIndex, i);
			const auto* const metadataIt = m_metadata + location;

			if (auto match = metadataIt->match(shortHash); match != 0) {
				const auto* const it = m_array + location * 15;

				do {
					auto toNext = std::countr_zero(match);
					match &= match - 1;

					if LSD_UNORDERED_FLAT_IS_SET {
						if (auto candidate = it + toNext; m_equal(*candidate, key))
							return const_iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
					} else if (auto candidate = it + toNext; m_equal(candidate->first, key))
						return const_iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash)) return const_iterator { };
		}

		return const_iterator { };
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
		if (m_bucketCount == 0) return false;

		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		auto bucketIndex = hashToBucket(hash);

		for (size_type i = 0; i < m_bucketCount; i++) {
			auto location = prober(bucketIndex, i);
			const auto* metadataIt = m_metadata + location;

			if (auto match = metadataIt->match(shortHash); match != 0) {
				const auto* it = m_array + location * 15;

				do {
					auto toNext = std::countr_zero(match);
					match &= match - 1;

					if LSD_UNORDERED_FLAT_IS_SET {
						if (m_equal(it[toNext], key)) return true;
					} else if (m_equal(it[toNext].first, key)) return true;
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash)) return false;
		}

		return false;
	}

	template <class K> [[nodiscard]] constexpr size_type count(const K& key) const noexcept {
		if (m_bucketCount == 0) return false;

		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		auto bucketIndex = hashToBucket(hash);

		for (size_type i = 0; i < m_bucketCount; i++) {
			auto location = prober(bucketIndex, i);
			const auto* metadataIt = m_metadata + location;

			if (auto match = metadataIt->match(shortHash); match != 0) {
				const auto* it = m_array + bucketIndex * 15;

				do {
					auto toNext = std::countr_zero(match);
					match &= match - 1;

					if LSD_UNORDERED_FLAT_IS_SET {
						if (m_equal(it[toNext], key)) return 1;
					} else if (m_equal(it[toNext].first, key)) return 1;
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash)) return 0;
		}

		return 0;
	}


	template <class K> [[nodiscard]] constexpr auto& at(const K& key) {
		auto it = find(key);
		if (it.m_pointer == nullptr) throw std::out_of_range("lsd::BasicUnorderedFlat::at(): Specified key could not be found in container!");

		if LSD_UNORDERED_FLAT_IS_SET return *it;
		else return it->second;
	}
	template <class K> [[nodiscard]] constexpr const auto& at(const K& key) const {
		auto it = find(key);
		if (it.m_pointer == nullptr) throw std::out_of_range("lsd::BasicUnorderedFlat::at(): Specified key could not be found in container!");

		if LSD_UNORDERED_FLAT_IS_SET return *it;
		else return it->second;
	}

	[[nodiscard]] constexpr auto& operator[](const key_type& key) {
		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);
		auto it = find(hash, shortHash, bucketIndex, key);

		if LSD_UNORDERED_FLAT_IS_SET
			return (it.m_pointer == nullptr) ? *basicInsert(hash, shortHash, bucketIndex, key) : *it;
		else
			return (it.m_pointer == nullptr) ? basicEmplace(hash, shortHash, bucketIndex, key, mapped_type())->second : it->second;
	}
	template <class K> [[nodiscard]] constexpr auto& operator[](K&& key) {
		const auto hash = postMixOrHash(key);
		const auto shortHash = metadata_group::hashToMetadata(hash);
		const auto bucketIndex = hashToBucket(hash);
		auto it = find(hash, shortHash, bucketIndex, key);

		if LSD_UNORDERED_FLAT_IS_SET
			return (it.m_pointer == nullptr) ? *basicInsert(hash, shortHash, bucketIndex, std::forward<K>(key)) : *it;
		else
			return (it.m_pointer == nullptr) ? basicEmplace(hash, shortHash, bucketIndex, std::forward<K>(key), mapped_type())->second : it->second;
	}


	// Getters

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_size;
	}
	[[nodiscard]] constexpr size_type maxSize() const noexcept {
		return maxBucketCount() * 15;
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_size() const noexcept {
		return maxSize();
	}
	[[nodiscard]] constexpr bool empty() const noexcept {
		return m_size == 0;
	}

	[[nodiscard]] constexpr allocator_type allocator() const noexcept {
		return m_alloc;
	}
	[[deprecated]] [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
		return allocator();
	}

	[[nodiscard]] constexpr size_type bucketCount() const noexcept {
		return m_bucketCount;
	}
	[[deprecated]] [[nodiscard]] constexpr size_type bucket_count() const noexcept {
		return bucketCount();
	}
	[[nodiscard]] constexpr size_type maxBucketCount() const noexcept {
		return size_type { 1 } << (sizeof(size_type) * 8 - std::countl_zero(
			std::min(metadata_allocator_traits::max_size(m_metadataAlloc),
					 allocator_traits::max_size(m_alloc) / 15)
		));
	}
	[[deprecated]] [[nodiscard]] constexpr size_type max_bucket_count() const noexcept {
		return maxBucketCount();
	}
	[[nodiscard]] consteval size_type bucketSize() const noexcept {
		return metadata_group::bucketSize;
	}
	[[deprecated]] [[nodiscard]] consteval size_type bucket_size(size_type) const noexcept {
		return bucketSize();
	}

	[[nodiscard]] constexpr metadata_allocator_type metadataAllocator() const noexcept {
		return m_metadataAlloc;
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
		return m_size / (m_bucketCount * 15);
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
	metadata_pointer m_metadata { };
	pointer m_array { };

	size_type m_size { };
	size_type m_bucketCount { };

	size_type m_loadFactor { };

	[[no_unique_address]] hasher m_hasher { };
	[[no_unique_address]] key_equal m_equal { };

	[[no_unique_address]] allocator_type m_alloc { };
	[[no_unique_address]] metadata_allocator_type m_metadataAlloc { };


	// Utility and base functions

	template <class K> constexpr size_type postMixOrHash(const K& key) const noexcept {
		if constexpr (hashRequiresPostMixValue<hasher>) {
#if UINTPTR_MAX == UINT64_MAX
			static constexpr std::size_t bHi = std::size_t { 0x9E3779B97F4A7C15 } >> 32;
			static constexpr std::size_t bLo = std::size_t { 0x9E3779B97F4A7C15 } &0xFFFFFFFF;

			auto hLo = m_hasher(key);
			std::size_t hHi = hLo >> 32;
			hLo &= 0xFFFFFFFF;

			auto p0 = hLo * bLo;
			auto p1 = hHi * bHi;
			auto cross = (hLo + hHi) * (bLo + bHi) - p1 - p0;

			auto low = p0 + (cross << 32);

			return low ^ (p1 + (cross >> 32) + (low < p0)); // Discards a overflow byte
#elif UINTPTR_MAX == UINT32_MAX
			std::uint64_t r = static_cast<std::uint64_t>(i) * 0xE817FB2D;
			return static_cast<size_t>(r) ^ static_cast<size_t>(r >> 32);
#else
#error "lsd::unordered_flat::BasicUnorderedFlat::operator(): Container only supports systems with 64-bit or 32-bit word size!"
			return 0;
#endif
		} else return m_hasher(key);
	}
	constexpr size_type valueToHash(const value_type& v) {
		if LSD_UNORDERED_FLAT_IS_SET return postMixOrHash(v);
		else return postMixOrHash(v.first);
	}
	constexpr size_type prober(size_type base, size_type index) const noexcept {
		return (base + (index * (index + 1) >> 1)) & (m_bucketCount - 1);
	}
	constexpr size_type hashToBucket(size_type hash) const noexcept {
		return (m_bucketCount == 1) ? 0 : (hash >> (std::countl_zero(m_bucketCount) + 1));
	}


	template <class K> [[nodiscard]] constexpr iterator find(size_type hash, size_type shortHash, size_type bucketIndex, const K& key) noexcept {
		if (m_bucketCount == 0) return iterator { };

		for (size_type i = 0; i < m_bucketCount; i++) {
			auto location = prober(bucketIndex, i);
			const auto* metadataIt = m_metadata + location;

			if (auto match = metadataIt->match(shortHash); match != 0) {
				auto it = m_array + location * 15;

				do {
					auto toNext = std::countr_zero(match);
					match &= match - 1;

					if LSD_UNORDERED_FLAT_IS_SET {
						if (auto candidate = it + toNext; m_equal(*candidate, key))
							return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
					} else if (auto candidate = it + toNext; m_equal(candidate->first, key))
						return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + toNext, candidate };
				} while (match != 0);
			}

			if (!metadataIt->overflowed(shortHash)) return iterator { };
		}

		return iterator { };
	}

	constexpr pointer insertShortHashAndGetRawIt(size_type shortHash, size_type bucketIndex) noexcept {
		auto metadataIt = m_metadata + bucketIndex;
		auto occupied = metadataIt->matchOccupied();

		size_type location = bucketIndex;
		for (size_type i = 1; occupied == metadata_group::full && i < m_bucketCount; i++) {
			metadataIt->markOverflow(shortHash);
			metadataIt = m_metadata + (location = prober(bucketIndex, i));

			occupied = metadataIt->matchOccupied();
		}

		// Group found with empty elements still available
		auto groupIndex = std::countr_one(occupied);
		metadataIt->insert(groupIndex, shortHash);

		return m_array + location * 15 + groupIndex;
	}
	constexpr iterator insertShortHashAndGetIterator(size_type shortHash, size_type bucketIndex) noexcept {
		auto metadataIt = m_metadata + bucketIndex;
		auto occupied = metadataIt->matchOccupied();

		size_type location = bucketIndex;
		for (size_type i = 1; occupied == metadata_group::full && i < m_bucketCount; i++) {
			metadataIt->markOverflow(shortHash);
			metadataIt = m_metadata + (location = prober(bucketIndex, i));

			occupied = metadataIt->matchOccupied();
		}

		// Group found with empty elements still available
		auto groupIndex = std::countr_one(occupied);
		metadataIt->insert(groupIndex, shortHash);

		return iterator { LSD_UNORDERED_FLAT_TO_BYTE(metadataIt) + groupIndex, m_array + location * 15 + groupIndex };
	}


	constexpr void basicRehash(size_type bucketCount) {
		if (bucketCount > maxBucketCount()) throw std::length_error("lsd::BasicUnorderedFlat::basicRehash(): Requested size larger than the maximum size of the container!");
		std::swap(m_bucketCount, bucketCount); // Some functions down the line have m_bucketCount as a dependency

		auto oldMetadata = std::exchange(m_metadata, metadata_allocator_traits::allocate(m_metadataAlloc, m_bucketCount));
		auto oldArray = std::exchange(m_array, allocator_traits::allocate(m_alloc, m_bucketCount * 15));

		// Initialize all metadata groups
		for (auto metadataIt = m_metadata, end = m_metadata + m_bucketCount; metadataIt != end; metadataIt++)
			allocator_traits::construct(m_alloc, metadataIt);

		// Set sentinel at end
		(m_metadata + (m_bucketCount - 1))->insertSentinel();

		for (auto it = iterator(oldMetadata, oldArray); it.m_pointer != nullptr; it++) {
			const auto hash = valueToHash(*it);
			const auto shortHash = metadata_group::hashToMetadata(hash);

			allocator_traits::construct(m_alloc, insertShortHashAndGetRawIt(shortHash, hashToBucket(hash)), std::move(*it));
			allocator_traits::destroy(m_alloc, it.m_pointer);
		}

		m_loadFactor = (maxLFactor * m_bucketCount) * 15;

		metadata_allocator_traits::deallocate(m_metadataAlloc, oldMetadata, bucketCount);
		allocator_traits::deallocate(m_alloc, oldArray, bucketCount * 15);
	}
	constexpr void basicInplaceRehash() {
		// Reset all overflow bytes
		for (auto metadataIt = m_metadata, end = m_metadata + m_bucketCount; metadataIt != end; metadataIt++)
			metadataIt->clearOverflow();

		for (auto it = begin(); it.m_pointer != nullptr; it++) {
			const auto hash = valueToHash(*it);
			const auto shortHash = metadata_group::hashToMetadata(hash);

			size_type bucketIndex = hashToBucket(hash);
			auto metadataIt = m_metadata + bucketIndex;

			auto diff = reinterpret_cast<uintptr_t>(it.m_metadata) & metadata_group::bucketSize;
			auto metadataFromIt = reinterpret_cast<metadata_pointer>(const_cast<metadata_group::byte_type*>(it.m_metadata) - diff);
			if (metadataFromIt != metadataIt) { // Attempts to find a new location for values not in the original bucket
				metadataFromIt->erase(diff);

				if (auto newIt = insertShortHashAndGetRawIt(shortHash, bucketIndex); newIt != it.m_pointer) {
					allocator_traits::construct(m_alloc, newIt, std::move(*it));
					allocator_traits::destroy(m_alloc, it.m_pointer);
				}
			}
		}

		m_loadFactor = (maxLFactor * m_bucketCount) * 15;
	}


	constexpr iterator basicInsert(size_type hash, size_type shortHash, size_type bucketIndex, const value_type& value) {
		if (reserve(++m_size)) bucketIndex = hashToBucket(hash);

		auto it = insertShortHashAndGetIterator(shortHash, bucketIndex);
		allocator_traits::construct(m_alloc, it.m_pointer, value);

		return it;
	}
	constexpr iterator basicInsert(size_type hash, size_type shortHash, size_type bucketIndex, value_type&& value) {
		if (reserve(++m_size)) bucketIndex = hashToBucket(hash);

		auto it = insertShortHashAndGetIterator(shortHash, bucketIndex);
		allocator_traits::construct(m_alloc, it.m_pointer, std::move(value));

		return it;
	}
	template <class K, class... Args>
	constexpr iterator basicEmplace(size_type hash, size_type shortHash, size_type bucketIndex, K&& key, Args&&... args)
		LSD_UNORDERED_FLAT_REQUIRES_MAP
	{
		if (reserve(++m_size)) bucketIndex = hashToBucket(hash);

		auto it = insertShortHashAndGetIterator(shortHash, bucketIndex);
		allocator_traits::construct(m_alloc, it.m_pointer, std::forward<K>(key), mapped_type(std::forward<Args>(args)...));

		return it;
	}

	template <IteratorType It> constexpr void moveInsert(It first, It last) {
		for (; first != last; first++) insert(std::move(*first));
	}
};

} // namespace unordered_flat

} // namespace lsd
