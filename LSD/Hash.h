/*************************
 * @file Hash.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Common hash function implementations
 * 
 * @date 2024-03-02
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Operators.h"

#include <type_traits>
#include <typeindex>
#include <limits>
#include <functional>
#include <string>
#include <filesystem>

namespace lsd {

template <class> struct Hash;

template <class Ty> concept IntegralType = std::is_integral_v<Ty>;
template <class Ty> concept PointerType = std::is_pointer_v<Ty>;

template <IntegralType Integral> struct Hash<Integral> {
	constexpr std::size_t operator()(Integral i) const noexcept {
		if constexpr (sizeof(i) < sizeof(std::size_t) && std::is_signed_v<Integral>) {
			if (i >= 0) return static_cast<std::size_t>(i);
			else static_cast<std::size_t>(i) + static_cast<std::size_t>(std::numeric_limits<Integral>::max() / 2);
		} else if constexpr(sizeof(i) <= sizeof(std::size_t) && std::is_unsigned_v<Integral>) {
			return static_cast<std::size_t>(i);
		} else {
			i = (i ^ (i >> 30)) * Integral(0xbf58476d1ce4e5b9);
			i = (i ^ (i >> 27)) * Integral(0x94d049bb133111eb);
			return i ^ (i >> 31);
		}
	}
};

template <PointerType Pointer> struct Hash<Pointer> {
	std::size_t operator()(Pointer p) const noexcept {
		auto i = reinterpret_cast<std::size_t>(p);
		i = (i ^ (i >> 30)) * std::size_t(0xbf58476d1ce4e5b9);
		i = (i ^ (i >> 27)) * std::size_t(0x94d049bb133111eb);
		return i ^ (i >> 31);
	}
};

template <> struct Hash<std::nullptr_t> {
	constexpr std::size_t operator()(std::nullptr_t) const noexcept {
		return 0;
	}
};

template <> struct Hash<std::filesystem::path> {
	std::size_t operator()(const std::filesystem::path& p) const noexcept {
		return std::filesystem::hash_value(p);
	}
};

template <> struct Hash<std::string> {
	std::size_t operator()(const std::string& p) const noexcept {
		return std::hash<std::string>()(p);
	}
};

template <> struct Hash<std::wstring> {
	std::size_t operator()(const std::wstring& p) const noexcept {
		return std::hash<std::wstring>()(p);
	}
};

template <EnumType Enum> struct Hash<Enum> {
	constexpr std::size_t operator()(Enum e) const noexcept {
		return static_cast<std::size_t>(e);
	}
};

} // namespace lsd

#define CUSTOM_HASHER(name, type, hashType, hasher, toHashType)\
class name {\
public:\
	constexpr std::size_t operator()(type ty) const noexcept {\
		return hasher((ty)toHashType);\
	}\
	constexpr std::size_t operator()(hashType hash) const noexcept {\
		return hasher(hash);\
	}\
};

#define CUSTOM_EQUAL(name, type, hashType, toHashType)\
class name {\
public:\
	constexpr bool operator()(type first, type second) const noexcept {\
		return (first)toHashType == (second)toHashType;\
	}\
	constexpr bool operator()(type first, hashType second) const noexcept {\
		return (first)toHashType == second;\
	}\
	constexpr bool operator()(hashType first, type second) const noexcept {\
		return first == (second)toHashType;\
	}\
	constexpr bool operator()(hashType first, hashType second) const noexcept {\
		return first == second;\
	}\
};
