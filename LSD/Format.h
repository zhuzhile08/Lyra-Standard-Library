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
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/FromChars/FromCharsIntegral.h"
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

// formatting functions

template <class... Args> inline String format(FormatString<Args...> fmt, Args&&... args) {
	String out;
	FormatContext(
		detail::FormatBackInserter(
			&out,
			[](void* out, const char& v) { static_cast<String*>(out)->pushBack(v); },
			[](void* out) { return false; }
		),
		makeFormatArgs(args...)
	).format(fmt.get());

	return out;
}
template <class... Args> inline WString format(WFormatString<Args...> fmt, Args&&... args) {
	WString out;
	WFormatContext(
		detail::FormatBackInserter(
			&out,
			[](void* out, const wchar_t& v) { static_cast<WString*>(out)->pushBack(v); },
			[](void* out) { return false; }
		),
		makeWFormatArgs(args...)
	).format(fmt.get());

	return out;
}


template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt format_to(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt format_to(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	
}


template <class... Args> inline void print(FormatString<Args...> fmt, Args&&... args) {
	print(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void print(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class... Args> inline void println(FormatString<Args...> fmt, Args&&... args) {
	println(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void println(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}

} // namespace lsd
