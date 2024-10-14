/*************************
 * @file Formatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for all formatter classes
 * 
 * @date 2024-08-20
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"
#include "FormatArgs.h"

#include <cstddef>

namespace lsd {

namespace detail {

// formatter implementations

template <class CharT1, class CharT2> struct CharsFormatter {
	constexpr static void format(CharT1 c, detail::BasicFormatBackInserter<CharT2>& inserter, const detail::BasicFieldOptions<CharT1>& options) {
		switch (options.align) {
			case '<':
				if (!inserter.done()) inserter = c;
				for (auto count = options.fillCount; !inserter.done() && count > 1; count--) inserter = options.fillChr;

				break;

			case '>':
				for (auto count = options.fillCount; !inserter.done() && count > 1; count--) inserter = options.fillChr;
				if (!inserter.done()) inserter = c;

				break;

			case '^': {
				auto half = options.fillCount / 2;
				for (auto count = options.fillCount - half - 1; !inserter.done() && count > 0; count--) inserter = options.fillChr;
				if (!inserter.done()) inserter = c;
				for (; !inserter.done() && half > 0; half--) inserter = options.fillChr;

				break;
			}
		}
	}
};

template <class CharTy> struct StringFormatter {
	constexpr static void format(
		const char* data, 
		std::size_t length, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {

	}

	constexpr static void format(
		char* data, 
		std::size_t length, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {

	}
};

} // namespace detail


// character formatters

template <class CharTy> struct Formatter<char, CharTy> {
	constexpr void format(
		char c, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::CharsFormatter<char, CharTy>::format(c, inserter, options);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	constexpr void format(wchar_t c, detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		detail::CharsFormatter<wchar_t, wchar_t>::format(c, inserter, options);
	}
};


// bool formatter
template <class CharTy> struct Formatter<bool, CharTy> {
	constexpr void format(
		bool value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};


// string formatters

template <class CharTy> struct Formatter<CharTy*, CharTy> {
	constexpr void format(
		CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, std::strlen(value), inserter, options);
	}
};
template <class CharTy> struct Formatter<const CharTy*, CharTy> {
	constexpr void format(
		const CharTy* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, std::strlen(value), inserter, options);
	}
};

template <std::size_t Count, class CharTy> struct Formatter<CharTy[Count], CharTy> {
	constexpr void format(
		const CharTy (&value)[Count], 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value, Count, inserter, options);
	}
};

template <class Traits, class Alloc, class CharTy> struct Formatter<BasicString<CharTy, Traits, Alloc>, CharTy> {
	constexpr void format(
		const BasicString<CharTy, Traits, Alloc>& value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value.data(), value.size(), inserter, options);
	}
};
template <class Traits, class CharTy> struct Formatter<BasicStringView<CharTy, Traits>, CharTy> {
	constexpr void format(
		BasicStringView<CharTy, Traits> value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		detail::StringFormatter<CharTy>::format(value.data(), value.size(), inserter, options);
	}
};


// arithmatic formatters

template <class CharTy> struct Formatter<short, CharTy> {
	constexpr void format(
		short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned short, CharTy> {
	constexpr void format(
		unsigned short value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<int, CharTy> {
	constexpr void format(
		int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned int, CharTy> {
	constexpr void format(
		unsigned int value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long, CharTy> {
	constexpr void format(
		long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned long, CharTy> {
	constexpr void format(
		unsigned long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long long, CharTy> {
	constexpr void format(
		long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<unsigned long long, CharTy> {
	constexpr void format(
		unsigned long long value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};

template <class CharTy> struct Formatter<float, CharTy> {
	constexpr void format(
		float value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<double, CharTy> {
	constexpr void format(
		double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<long double, CharTy> {
	constexpr void format(
		long double value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};


// pointer formatters

template <class CharTy> struct Formatter<std::nullptr_t, CharTy> {
	constexpr void format(
		std::nullptr_t,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<void*, CharTy> {
	constexpr void format(
		void* value, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};
template <class CharTy> struct Formatter<const void*, CharTy> {
	constexpr void format(
		const void* value,
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		
	}
};


// special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<CharTy, detail::BasicFormatBackInserter<CharTy>>, CharTy> {
	constexpr void format(
		const detail::TypeErasedFormatArg<CharTy, detail::FormatBackInserter>& fmtArg, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& options) {
		fmtArg.format(inserter, options);
	}
};

} // namespace lsd
