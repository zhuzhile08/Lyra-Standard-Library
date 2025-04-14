/*************************
 * @file UnorderedFlatMap.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Unordered flat map
 * 
 * @date 2025-06-04
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/BasicUnorderedFlat.h"
#include "Detail/CoreUtility.h"

#include <memory>

namespace lsd {

template <
	class Key,
	class Ty,
	class Hash = Hash<Key>,
	class Equal = EqualTo<Key>,
	class Alloc = std::allocator<std::pair<Key, Ty>>
> using UnorderedFlatMap = unordered_flat::BasicUnorderedFlat<Key, Ty, Hash, Equal, Alloc>;

} // namespace lsd
