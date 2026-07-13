/************************
 * @file StringCore.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief String processing function implementations
 * 
 * @date 2026-05-20
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include <limits>
#include <iterator>


namespace lsd {

template <class, class, class> class BasicString;
template <class, class> class BasicStringView;


namespace string {

inline constexpr std::size_t strNPos { std::numeric_limits<std::size_t>::max() };


template <class CharTraits, std::input_iterator Iter>
constexpr int strcompare(Iter first, std::size_t firstSize, Iter second, std::size_t secondSize) noexcept {
	if (firstSize > secondSize) {
		auto r = CharTraits::compare(first, second, secondSize);

		if (r == 0)
			return 2;
		return r;
	}

	auto r = CharTraits::compare(first, second, firstSize);

	if (r == 0 && firstSize < secondSize)
		return -2;
	return r;
}


template <class CharTraits, std::random_access_iterator Iter>
constexpr bool strendsWith(Iter first, Iter firstEnd, Iter second, std::size_t secondSize) noexcept {
	return std::distance(first, firstEnd) >= secondSize && CharTraits::compare(firstEnd - secondSize, second, secondSize) == 0;
}

template <class CharTraits, std::random_access_iterator Iter>
constexpr bool strendsWith(Iter first, Iter firstEnd, typename CharTraits::char_type c) noexcept {
	return first != firstEnd && CharTraits::eq(*(firstEnd - 1), c);
}


template <class CharTraits, std::random_access_iterator Iter>
constexpr bool strcontains(Iter first, Iter firstEnd, Iter second, std::size_t secondSize) noexcept {
	if (secondSize == 0)
		return true;

	if (std::distance(first, firstEnd) < secondSize)
		return false;

	for (firstEnd = std::next(firstEnd - secondSize); first != firstEnd; first++)
		if (CharTraits::compare(first, second, secondSize) == 0)
			return true;

	return false;
}

template <class CharTraits, std::input_iterator Iter>
constexpr bool strcontains(Iter first, Iter firstEnd, typename CharTraits::char_type c) noexcept {
	for (; first != firstEnd; first++)
		if (CharTraits::eq(*first, c))
			return true;

	return false;
}


template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strfind(Iter begin, Iter first, Iter firstEnd, Iter second, std::size_t secondSize) noexcept {
	if (secondSize == 0)
		return first - begin;

	if (std::distance(first, firstEnd) < secondSize)
		return strNPos;

	for (firstEnd = std::next(firstEnd - secondSize); first != firstEnd; first++)
		if (CharTraits::compare(first, second, secondSize) == 0)
			return first - begin;

	return strNPos;
}

template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strfind(Iter begin, Iter first, Iter firstEnd, typename CharTraits::char_type c) noexcept {
	for (; first != firstEnd; first++)
		if (CharTraits::eq(*first, c))
			return first - begin;

	return strNPos;
}


template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strrfind(Iter begin, std::size_t firstSize, std::size_t pos, Iter second, std::size_t secondSize) noexcept {
	if (secondSize == 0)
		return pos;

	if (firstSize < secondSize)
		return strNPos;

	const auto end = std::prev(begin);
	for (auto rbegin = begin + std::min(pos, firstSize - secondSize); rbegin != end; rbegin--)
		if (CharTraits::compare(rbegin, second, secondSize) == 0)
			return rbegin - begin;

	return strNPos;
}

template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strrfind(Iter begin, std::size_t firstSize, std::size_t pos, typename CharTraits::char_type c) noexcept {
	if (firstSize < 1)
		return strNPos;

	const auto end = std::prev(begin);
	for (auto rbegin = begin + std::min(pos, firstSize - 1); rbegin != end; rbegin--)
		if (CharTraits::eq(*rbegin, c))
			return rbegin - begin;

	return strNPos;
}


template <class CharTraits, bool Negated, std::random_access_iterator Iter>
constexpr std::size_t strfindFirstOf(Iter begin, std::size_t firstSize, std::size_t pos, Iter otherBegin, Iter otherEnd) noexcept {
	if (pos >= firstSize || otherBegin == otherEnd)
		return strNPos;

	const auto end = begin + firstSize;
	for (auto it = begin + pos; it != end; it++) {
		bool found = false;

		for (auto oIt = otherBegin; oIt != otherEnd; oIt++) {
			if (CharTraits::eq(*it, *oIt)) {
				if constexpr (Negated) {
					found = true;
					break;
				} else
					return it - begin;
			}
		}

		if constexpr (Negated) if (!found)
			return it - begin;
	}

	return strNPos;
}

template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strfindFirstNotOf(Iter begin, std::size_t firstSize, std::size_t pos, typename CharTraits::char_type c) noexcept {
	if (pos >= firstSize)
		return strNPos;

	for (auto it = begin + pos, end = begin + firstSize; it != end; it++)
		if (!CharTraits::eq(*it, c))
			return it - begin;

	return strNPos;
}


template <class CharTraits, bool Negated, std::random_access_iterator Iter>
constexpr std::size_t strfindLastOf(Iter begin, std::size_t firstSize, std::size_t pos, Iter otherBegin, Iter otherEnd) noexcept {
	if (otherBegin == otherEnd)
		return strNPos;
	
	const auto end = std::prev(begin);
	for (auto it = begin + std::min(pos, firstSize - 1); it != end; it--) {
		bool found = false;

		for (auto oIt = otherBegin; oIt != otherEnd; oIt++) {
			if (CharTraits::eq(*it, *oIt)) {
				if constexpr (Negated) {
					found = true;
					break;
				} else
					return it - begin;
			}
		}

		if constexpr (Negated) if (!found)
			return it - begin;
	}

	return strNPos;
}

template <class CharTraits, std::random_access_iterator Iter>
constexpr std::size_t strfindLastNotOf(Iter begin, std::size_t firstSize, std::size_t pos, typename CharTraits::char_type c) noexcept {
	const auto end = std::prev(begin);
	for (auto it = begin + std::min(pos, firstSize - 1); it != end; it--)
		if (!CharTraits::eq(*it, c))
			return it - begin;

	return strNPos;
}

} // namespace string

} // namespace lsd
