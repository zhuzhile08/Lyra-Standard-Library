/*************************
 * @file UnorderedDenseSet.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Unordered Sparse Set implementation
 * 
 * @date 2024-02-24
 * 
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
> using UnorderedDenseSet = detail::BasicUnorderedDense<Key, void, Hash, Equal, Alloc>;

} // namespace lsd
