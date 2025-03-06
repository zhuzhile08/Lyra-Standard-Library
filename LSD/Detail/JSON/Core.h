/**************************
 * @file Core.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Core utilities for the JSON class
 * 
 * @date 2025-03-06
 * @copyright Copyright (c) 2025
 **************************/

#pragma once

#include "../../String.h"

#include <cstdint>
#include <exception>

namespace lsd {

class JsonParseError : public std::runtime_error {
public:
	JsonParseError(const String& message) : std::runtime_error(message.cStr()) {
		m_message.append(message);
	}
	JsonParseError(const char* message) : std::runtime_error(message) {
		m_message.append(message);
	}
	JsonParseError(const JsonParseError&) = default;
	JsonParseError(JsonParseError&&) = default;

	JsonParseError& operator=(const JsonParseError&) = default;
	JsonParseError& operator=(JsonParseError&&) = default;

	const char* what() const noexcept override {
		return m_message.cStr();
	}

private:
	String m_message { "Program terminated with JsonParseError: " };
};

struct JsonNull { };
struct JsonObject { };

namespace detail {

// Format helper classes

template <class StringType> class StringifyFormatHelper {
public:
	constexpr void beginObject(StringType& s) const {
		s.pushBack('{');
	}
	constexpr void endObject(StringType& s) const {
		s.pushBack('}');
	}

	constexpr void beginArray(StringType& s) const {
		s.pushBack('[');
	}
	constexpr void endArray(StringType& s) const {
		s.pushBack(']');
	}

	constexpr void seperator(bool condition, StringType& s) const {
		if (condition) s.pushBack(',');
	}
};

template <class StringType> class PrettyStringifyFormatHelper {
public:
	constexpr void beginObject(StringType& s) {
		++m_indent;
		s.append("{\n");
	}
	constexpr void endObject(StringType& s) {
		(s += '\n').append(--m_indent, '\t').pushBack('}');
	}

	constexpr void beginArray(StringType& s) {
		++m_indent;
		s.append("[\n");
	}
	constexpr void endArray(StringType& s) {
		(s += '\n').append(--m_indent, '\t').pushBack(']');
	}

	constexpr void seperator(bool condition, StringType& s) const {
		if (condition) s.append(",\n");
		s.append(m_indent, '\t');
	}

private:
	std::size_t m_indent = 0;
};

} // namespace detail

} // namespace lsd
