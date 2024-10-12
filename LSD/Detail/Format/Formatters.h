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

template <class CharT1, class CharT2> struct CharsFormatter {
	constexpr static void format(CharT1 c, detail::BasicFormatBackInserter<CharT2>& inserter, const detail::BasicFieldOptions<CharT1>& options) {
		switch (options.align) {
			case '<':
				if (!inserter.done()) inserter = c;
				for (auto count = options.fillCount; !inserter.done() && count > 1; count--) inserter = options.fillChr;

				break;

			case '=':
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

} // namespace detail


// character formatters

template <> struct Formatter<char, char> {
	constexpr void format(char c, detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		detail::CharsFormatter<char, char>::format(c, inserter, options);
	}
};
template <> struct Formatter<char, wchar_t> {
	constexpr void format(char c, detail::WFormatBackInserter& inserter, const detail::FieldOptions& options) {
		detail::CharsFormatter<char, wchar_t>::format(c, inserter, options);
	}
};
template <> struct Formatter<wchar_t, wchar_t> {
	constexpr void format(wchar_t c, detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		detail::CharsFormatter<wchar_t, wchar_t>::format(c, inserter, options);
	}
};


// string formatters

template <> struct Formatter<char*, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<wchar_t*, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};

template <> struct Formatter<const char*, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<const wchar_t*, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};

template <std::size_t Count> struct Formatter<char[Count], char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <std::size_t Count> struct Formatter<wchar_t[Count], wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};

template <class Traits, class Alloc> struct Formatter<BasicString<char, Traits, Alloc>, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <class Traits, class Alloc> struct Formatter<BasicString<wchar_t, Traits, Alloc>, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};

template <class Traits> struct Formatter<BasicStringView<char, Traits>, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <class Traits> struct Formatter<BasicStringView<wchar_t, Traits>, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};


// arithmatic formatters

template <> struct Formatter<short int, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<unsigned short int, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<int, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<unsigned int, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<long int, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<unsigned long int, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<long long int, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<unsigned long long int, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};

template <> struct Formatter<float, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<float, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<double, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<double, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<long double, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<long double, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};


// pointer formatters

template <> struct Formatter<std::nullptr_t, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<std::nullptr_t, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<void*, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<void*, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};
template <> struct Formatter<const void*, char> {
	constexpr void format(detail::FormatBackInserter& inserter, const detail::FieldOptions& options) {
		
	}
};
template <> struct Formatter<const void*, wchar_t> {
	constexpr void format(detail::WFormatBackInserter& inserter, const detail::WFieldOptions& options) {
		
	}
};


// special type erased argument store formatters

template <> struct Formatter<detail::TypeErasedFormatArg<char, detail::WFormatBackInserter>, char> {
	constexpr void format(
		const detail::TypeErasedFormatArg<char, detail::FormatBackInserter>& fmtArg, 
		detail::FormatBackInserter& inserter, 
		const detail::FieldOptions& options) {
		fmtArg.format(inserter, options);
	}
};

template <> struct Formatter<detail::TypeErasedFormatArg<wchar_t, detail::WFormatBackInserter>, wchar_t> {
	constexpr void format(
		const detail::TypeErasedFormatArg<wchar_t, detail::WFormatBackInserter>& fmtArg, 
		detail::WFormatBackInserter& inserter, 
		const detail::WFieldOptions& options) {
		fmtArg.format(inserter, options);
	}
};

} // namespace lsd
