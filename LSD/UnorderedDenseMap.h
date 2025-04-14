/*************************
 * @file UnorderedDenseMap.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Unordered dense map
 * 
 * @date 2024-02-24
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/BasicUnorderedDense.h"
#include "Detail/CoreUtility.h"

#include <memory>

namespace lsd {

template <
	class Key,
	class Ty,
	class Hash = Hash<Key>,
	class Equal = EqualTo<Key>,
	class Alloc = std::allocator<std::pair<Key, Ty>>
> using UnorderedDenseMap = unordered_dense::BasicUnorderedDense<Key, Ty, Hash, Equal, Alloc>;

} // namespace lsd
