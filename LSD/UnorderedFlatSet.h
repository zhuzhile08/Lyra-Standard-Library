/*************************
 * @file UnorderedFlatSet.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Unordered flat set
 * 
 * @date 2025-06-04
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/BasicUnorderedDense.h"
#include "Detail/CoreUtility.h"

#include <memory>

namespace lsd {

template <
	class Key,
	class Hash = Hash<Key>,
	class Equal = EqualTo<Key>,
	class Alloc = std::allocator<Key>
> using UnorderedFlatSet = unordered_flat::BasicUnorderedFlat<Key, void, Hash, Equal, Alloc>;

} // namespace lsd
