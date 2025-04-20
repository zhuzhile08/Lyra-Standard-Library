/*************************
 * @file Format.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Formatting utility and library
 * 
 * @brief Formatting specification: 
 * @brief replacementField  ::= "{"[field][":"formatSpec]"}"
 * @brief formatSpec		::= <view in Detail/Format/FormatSpecs.h>
 * 
 * @date 2024-06-18
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/FromChars/Integral.h"
#include "Detail/Format/FormatContext.h"
#include "Detail/Format/Formatters.h"
#include "Detail/Format/FormatArgs.h"
#include "Detail/Format/FormatCore.h"

#include "Iterators.h"
#include "Array.h"
#include "String.h"
#include "StringView.h"
#include "UniquePointer.h"
#include "FunctionPointer.h"
#include "Utility.h"

#include <cctype>
#include <type_traits>
#include <concepts>

namespace lsd {

// Formatting functions

template <class... Args> inline String format(FormatString<Args...> fmt, Args&&... args) {
	String out;
	FormatContext(
		detail::FormatBackInserter(
			&out,
			[](void* out, const char& v) { static_cast<String*>(out)->pushBack(v); },
			[](void*, std::size_t&) { return false; }
		),
		makeFormatArgs(args...)
	).format(fmt.get());

	return out;
}
template <class... Args> inline WString format(WFormatString<Args...> fmt, Args&&... args) {
	WString out;
	WFormatContext(
		detail::WFormatBackInserter(
			&out,
			[](void* out, const wchar_t& v) { static_cast<WString*>(out)->pushBack(v); },
			[](void*, std::size_t&) { return false; }
		),
		makeWFormatArgs(args...)
	).format(fmt.get());

	return out;
}


template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	FormatContext cxt(
		detail::FormatBackInserter(
			&it,
			[](void* it, const char& v) { 
				auto out = static_cast<OutputIt*>(it);
				*out = v;
				++*out;
			},
			[](void*, std::size_t& n) { return n-- > 1; },
			n
		),
		makeFormatArgs(args...)
	);

	cxt.format(fmt.get());
	return cxt.out().template get<OutputIt>();
}
template <class OutputIt, class... Args> [[deprecated]] inline OutputIt format_to(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	formatTo(it, n, fmt, std::forward<Args>(args)...);
}
template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	WFormatContext cxt(
		detail::WFormatBackInserter(
			&it,
			[](void* it, const char& v) { 
				auto out = static_cast<OutputIt*>(it);
				*out = v;
				++*out;
			},
			[](void*, std::size_t& n) { return n-- > 1; },
			n
		),
		makeWFormatArgs(args...)
	);

	cxt.format(fmt.get());
	return cxt.out().template get<OutputIt>();
}
template <class OutputIt, class... Args> [[deprecated]] inline OutputIt format_to(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	formatTo(it, n, fmt, std::forward<Args>(args)...);
}


template <class... Args> inline void print(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	FormatContext(
		detail::FormatBackInserter(
			&stream,
			[](void* stm, const char& v) { std::putc(v, *static_cast<std::FILE**>(stm)); },
			[](void* stm, std::size_t&) { return std::ferror(*static_cast<std::FILE**>(stm)) == 0; }
		),
		makeFormatArgs(args...)
	).format(fmt.get());
}
template <class... Args> inline void print(FormatString<Args...> fmt, Args&&... args) {
	print(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void println(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	FormatContext(
		detail::FormatBackInserter(
			&stream,
			[](void* stm, const char& v) { std::putc(v, *static_cast<std::FILE**>(stm)); },
			[](void* stm, std::size_t&) { return std::ferror(*static_cast<std::FILE**>(stm)) != 0; }
		),
		makeFormatArgs(args...)
	).format(fmt.get());

	std::putc('\n', stream); // @todo Probably not the proper way to do this but it works
}
template <class... Args> inline void println(FormatString<Args...> fmt, Args&&... args) {
	println(stdout, fmt, std::forward<Args>(args)...);
}

} // namespace lsd
