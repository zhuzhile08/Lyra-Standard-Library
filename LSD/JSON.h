/**************************
 * @file JSON.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief A JSON parser and writer
 * 
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 **************************/

#pragma once

#include "Utility.h"
#include "UniquePointer.h"
#include "SharedPointer.h"
#include "Node.h"
#include "Vector.h"
#include "UnorderedSparseMap.h"

#include <exception>
#include <variant>
#include <charconv>

namespace lsd {

class JsonParseError : public std::runtime_error {
public:
	JsonParseError(const String& message) : std::runtime_error(message.cStr()) {
		m_message.append(message).pushBack('!');
	}
	JsonParseError(const char* message) : std::runtime_error(message) {
		m_message.append(message).pushBack('!');
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


namespace detail {

template <class Ty> concept LiteralType = std::is_integral_v<Ty>;
template <class Ty> concept SignedType = std::is_signed_v<Ty> && std::is_integral_v<Ty>;
template <class Ty> concept UnsignedType = std::is_unsigned_v<Ty> && std::is_integral_v<Ty>;
template <class Ty> concept FloatingType = std::is_floating_point_v<Ty>;

} // namespace detail


struct JsonNull { };
struct JsonObject { };


template <
	detail::LiteralType Literal = char,
	template <class...> class ArrayContainer = Vector,
	detail::SignedType Signed = std::int64_t,
	detail::UnsignedType Unsigned = std::uint64_t,
	detail::FloatingType Floating = double,
	template <class...> class NodeContainer = UnorderedSparseSet,
	template <class...> class SmartPointer = UniquePointer> 
class BasicJson : public BasicNode<
	BasicJson<Literal, ArrayContainer, Signed, Unsigned, Floating, NodeContainer, SmartPointer>, 
	SmartPointer,
	BasicString<Literal>, 
	NodeContainer> {
public:
	using size_type = std::size_t;
	using null_type = JsonNull;
	using object_type = JsonObject;
	using signed_type = Signed;
	using unsigned_type = Unsigned;
	using floating_type = Floating;
	using literal_type = Literal;
	using string_type = BasicString<literal_type>;

	using json_type = BasicJson;
	using reference = json_type&;
	using const_reference = const json_type&;
	using rvreference = json_type&&;
	using smart_pointer = SmartPointer<json_type>;

	using array_type = ArrayContainer<smart_pointer>;
	using node_type = BasicNode<json_type, SmartPointer, string_type, NodeContainer>;
	using value_type = std::variant<
		null_type,
		bool,
		object_type,
		unsigned_type,
		signed_type,
		floating_type,
		array_type,
		string_type
	>;

	constexpr BasicJson() noexcept = default;
	constexpr BasicJson(const value_type& value) noexcept requires std::is_copy_assignable_v<smart_pointer> : m_value(value) { }
	constexpr BasicJson(value_type&& value) noexcept : m_value(std::move(value)) { }
	template <class KeyType> 
	constexpr BasicJson(KeyType&& key, value_type&& value) noexcept : node_type(std::forward<KeyType>(key)), m_value(std::move(value)) { }
	template <class KeyType, class Value> 
	constexpr BasicJson(KeyType&& key, Value&& value) noexcept : node_type(std::forward<KeyType>(key)) {
		assign(std::forward<Value>(value));
	}
	constexpr BasicJson(const BasicJson&) requires std::is_copy_assignable_v<smart_pointer> = default;
	constexpr BasicJson(BasicJson&&) = default;
	constexpr ~BasicJson() noexcept = default;

	constexpr reference operator=(const_reference) = default;
	constexpr reference operator=(rvreference) = default;

	constexpr reference operator=(const value_type& value) noexcept {
		m_value = value;
		return *this;
	}
	constexpr reference operator=(value_type&& value) noexcept {
		m_value = std::move(value);
		return *this;
	}
	template <class Value> constexpr reference operator=(const Value& value) noexcept { 
		assign(value);
		return *this;
	}
	template <class Value> constexpr reference operator=(Value&& value) noexcept {
		assign(std::forward<Value>(value));
		return *this;
	}
	
	template <class Ty, std::enable_if_t<
		std::is_floating_point_v<std::remove_cvref_t<Ty>> ||
		std::is_unsigned_v<std::remove_cvref_t<Ty>> ||
		std::is_signed_v<std::remove_cvref_t<Ty>> ||
		std::is_nothrow_convertible_v<Ty, value_type>,
	int> = 0>
	constexpr void assign(const Ty& value) noexcept {
		if constexpr (std::is_floating_point_v<std::remove_cvref_t<Ty>>) m_value = static_cast<floating_type>(value);
		else if constexpr (std::is_unsigned_v<std::remove_cvref_t<Ty>>) m_value = static_cast<unsigned_type>(value);
		else if constexpr (std::is_signed_v<std::remove_cvref_t<Ty>>) m_value = static_cast<signed_type>(value);
		else m_value = value;
	}
	template <class Ty, std::enable_if_t<
		std::is_floating_point_v<std::remove_cvref_t<Ty>> ||
		std::is_unsigned_v<std::remove_cvref_t<Ty>> ||
		std::is_signed_v<std::remove_cvref_t<Ty>> ||
		std::is_nothrow_convertible_v<Ty, value_type>,
	int> = 0>
	constexpr void assign(Ty&& value) noexcept {
		if constexpr (std::is_floating_point_v<std::remove_cvref_t<Ty>>) m_value = static_cast<floating_type>(value);
		else if constexpr (std::is_unsigned_v<std::remove_cvref_t<Ty>>) m_value = static_cast<unsigned_type>(value);
		else if constexpr (std::is_signed_v<std::remove_cvref_t<Ty>>) m_value = static_cast<signed_type>(value);
		else m_value = std::forward<Ty>(value);
	}
	template <class Ty, class KeyType> constexpr void assign(KeyType&& key, const Ty& value) noexcept {
		assign(value);
		rename(std::forward<KeyType>(key));
	}
	template <class Ty, class KeyType> constexpr void assign(KeyType&& key, Ty&& value) noexcept {
		assign(value);
		rename(std::forward<KeyType>(key));
	}

	template <class Iterator> NODISCARD static constexpr json_type parse(Iterator begin, Iterator end) {
		// first node
		json_type json;

		skipCharacters(begin, end);

		// start parsing
		if (*begin == '{') json.m_value = parseObject(begin, end, json);
		else if (*begin == '[') json.m_value = parseArray(begin, end);
		else if (++begin == end) json.m_value = object_type();
		else throw JsonParseError("lsd::Json::parse(): JSON Syntax Error: Unexpected symbol, JSON file has to either contain a single object or array at global scope or be empty!");

		return json;
	}
	template <class Container> NODISCARD static constexpr json_type parse(const Container& container) {
		return parse(container.begin(), container.end());
	}

	template <class... Args> NODISCARD static constexpr smart_pointer create(Args&&... args) {
		return smart_pointer::create(std::forward<Args>(args)...);
	}

	constexpr string_type stringify() const {
		string_type r;

		if (isObject()) stringifyObject(*this, r);
		else if (isArray()) stringifyArray(*this, r);
		else stringifyPair(*this, r);

		return r;
	}
	constexpr string_type stringifyPretty() const {
		string_type r;

		if (isObject()) stringifyObjectPretty(0, *this, r);
		else if (isArray()) stringifyArrayPretty(0, *this, r);
		else stringifyPairPretty(0, *this, r);

		return r;
	}
	
	constexpr bool isObject() const noexcept {
		return std::holds_alternative<object_type>(m_value);
	}
	constexpr bool isArray() const noexcept {
		return std::holds_alternative<array_type>(m_value);
	}
	constexpr bool isString() const noexcept {
		return std::holds_alternative<string_type>(m_value);
	}
	constexpr bool isSigned() const noexcept {
		return std::holds_alternative<signed_type>(m_value);
	}
	constexpr bool isUnsigned() const noexcept {
		return std::holds_alternative<unsigned_type>(m_value);
	}
	constexpr bool isInteger() const noexcept {
		return isSigned() || isUnsigned();
	}
	constexpr bool isFloating() const noexcept {
		return std::holds_alternative<floating_type>(m_value);
	}
	constexpr bool isNumber() const noexcept {
		return isInteger() || isFloating();
	}
	constexpr bool isBoolean() const noexcept {
		return std::holds_alternative<bool>(m_value);
	}
	constexpr bool isNull() const noexcept {
		return std::holds_alternative<null_type>(m_value);
	}

	constexpr auto& boolean() noexcept {
		return std::get<bool>(m_value);
	}
	constexpr auto& sInt() noexcept {
		return std::get<signed_type>(m_value);
	}
	constexpr auto& uInt() noexcept {
		return std::get<unsigned_type>(m_value);
	}
	constexpr auto& floating() noexcept {
		return std::get<floating_type>(m_value);
	}
	constexpr auto& object() noexcept {
		return *this;
	}
	constexpr auto& array() noexcept {
		return std::get<array_type>(m_value);
	}
	constexpr auto& string() noexcept {
		return std::get<string_type>(m_value);
	}

	constexpr const auto& boolean() const noexcept {
		return std::get<bool>(m_value);
	}
	constexpr const auto& signedInt() const noexcept {
		return std::get<signed_type>(m_value);
	}
	constexpr const auto& unsignedInt() const noexcept {
		return std::get<unsigned_type>(m_value);
	}
	constexpr const auto& floating() const noexcept {
		return std::get<floating_type>(m_value);
	}
	constexpr const auto& object() const noexcept {
		return *this;
	}
	constexpr const auto& array() const noexcept {
		return std::get<array_type>(m_value);
	}
	constexpr const auto& string() const noexcept {
		return std::get<string_type>(m_value);
	}

	template <class Ty, std::enable_if_t<
		std::is_same_v<std::remove_cvref_t<Ty>, json_type> ||
		std::is_floating_point_v<std::remove_cvref_t<Ty>> ||
		std::is_unsigned_v<std::remove_cvref_t<Ty>> ||
		std::is_signed_v<std::remove_cvref_t<Ty>> ||
		std::is_nothrow_convertible_v<literal_type*, Ty> ||
		std::is_nothrow_convertible_v<Ty, value_type>,
	int> = 0>
	constexpr decltype(auto) get() const noexcept {
		using type = std::remove_cvref_t<Ty>;

		if constexpr (std::is_same_v<type, json_type>) return *this;
		else if constexpr (std::is_same_v<type, null_type>) return std::get<type>(m_value);
		else if constexpr (std::is_same_v<type, bool>) return std::get<type>(m_value);
		else if constexpr (std::is_floating_point_v<type>) return static_cast<type>(std::get<floating_type>(m_value));
		else if constexpr (std::is_unsigned_v<type>) return static_cast<type>(std::get<unsigned_type>(m_value));
		else if constexpr (std::is_signed_v<type>) return static_cast<type>(std::get<signed_type>(m_value));
		else if constexpr (std::is_nothrow_convertible_v<const literal_type* const, Ty>) return std::get<string_type>(m_value).data();
		else return (std::get<type>(m_value));
	}

	const_reference at(size_type i) const {
		return *get<array_type>().at(i);
	}
	reference operator[](size_type i) {
		return *get<array_type>()[i];
	}

	using node_type::operator[];

private:
	value_type m_value;

	template <class Iterator> static constexpr int skipCharacters(Iterator& begin, Iterator& end) {
		for (; begin != end; begin++) {
			switch (*begin) {
				case ' ':
				case '\f':
				case '\n':
				case '\r':
				case '\t':
				case '\v':
				case '\0':
					break;
				
				default:
					return *begin;
			}
		}

		return *begin;
	}

	template <class Iterator> static constexpr string_type parseString(Iterator& begin, Iterator& end) {
		if (*begin != '\"') throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, expected quotation marks!");

		string_type r;

		for (++begin; begin != end; begin++) {
			switch (*begin) {
				case '\\':
					r.pushBack(*++begin);
					break;

				case '\"':
					return r;
					break;
				
				default:
					r.pushBack(*begin);
					break;
			}
		}

		throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Missing symbol, string not terminated!");
		return r;
	}
	template <class Iterator> static constexpr value_type parsePrimitive(Iterator& begin, Iterator& end) {
		switch(*begin) {
			case 't':
				begin += 3;
				return true;

				break;
			case 'f':
				begin += 4;
				return false;
				
				break;
			case 'n':
				begin += 3;
				return value_type();

				break;
			
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
					auto sequenceEnd = begin + 1;
					for (bool finished = false; sequenceEnd != end && !finished; sequenceEnd++) {
						switch (*sequenceEnd) { 
							case ' ':
							case '\f':
							case '\n':
							case '\r':
							case '\t':
							case '\v':
							case '\0': 
							case '}':
							case ',':
								finished = true;
								break;
							
							default:
								break;
						}
					}

					unsigned_type uRes { };

					if (fromChars(&*begin, &*sequenceEnd, uRes).ptr == &*sequenceEnd) return uRes;
					else {
						signed_type sRes = { };

						if (fromChars(&*begin, &*sequenceEnd, sRes).ptr == &*sequenceEnd) return sRes;
						else {
							floating_type fRes = { };
							
							if (fromChars(&*begin, &*sequenceEnd, fRes).ptr == &*sequenceEnd) return fRes;
							else throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, number could not be parsed!");
						}
					}
				}

				break;
			
			default:
				throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, couldn't match identifier with any type!");
				break;
		}

		return value_type();
	}
	template <class Iterator> static constexpr object_type parseObject(Iterator& begin, Iterator& end, json_type& json) {
		for (++begin; begin != end; begin++) {
			switch(skipCharacters(begin, end)) {
				default:
					json.insert(parsePair(begin, end));
					break;
					
				case '}':
					return object_type();
					break;
				case ',':
					break;
			}
		}

		throw JsonParseError("lsd::Json::parseObject(): JSON Syntax Error: Missing symbol, expected closing curly brackets to close object!");

		return object_type();
	}
	template <class Iterator> static constexpr array_type parseArray(Iterator& begin, Iterator& end) {
		array_type r;

		for (++begin; begin != end; begin++) {
			auto tok = smart_pointer::create();

			switch(skipCharacters(begin, end)) {
				case '{':
					tok->m_value = parseObject(begin, end, *tok);
					r.emplaceBack(tok.release());

					break;
				case '[':
					tok->m_value = parseArray(begin, end);
					r.emplaceBack(tok.release());

					break;
				case '\"':
					tok->m_value = parseString(begin, end);
					r.emplaceBack(tok.release());

					if (*(begin + 1) != ']') ++begin;

					break;
				case ']':
					return r;
					break;
				case '}':
				case ',':
					break;

				default:
					tok->m_value = parsePrimitive(begin, end);
					r.emplaceBack(tok.release());
					
					break;
			}
		}

		throw JsonParseError("lsd::Json::parseArray(): JSON Syntax Error: Missing symbol!");

		return r;
	}
	template <class Iterator> static constexpr json_type parsePair(Iterator& begin, Iterator& end) {
		json_type tok;

		tok.m_name = parseString(begin, end);
		++begin;
		if (skipCharacters(begin, end) != ':') throw JsonParseError("lsd::Json::parsePair(): JSON Syntax Error: Unexpected symbol, expected double colon after variable name!");
		++begin;

		switch(skipCharacters(begin, end)) {
			case '{':
				tok.m_value = parseObject(begin, end, tok);

				break;
			case '[':
				tok.m_value = parseArray(begin, end);

				break;
			case '\"':
				tok.m_value = parseString(begin, end);

				break;
			
			case '}':
			case ']':
				++begin;
				break;

			default:
				tok.m_value = parsePrimitive(begin, end);
				break;
		}

		return tok;
	}

	static constexpr void stringifyPrimitive(const json_type& t, string_type& s) {
		if (t.isBoolean()) {
			if (t == true) s.append("true");
			else s.append("false");
		} else if (t.isSigned()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(t.get<signed_type>()));
			else
				s.append(toWString(t.get<signed_type>()));
		} else if (t.isUnsigned()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(t.get<unsigned_type>()));
			else
				s.append(toWString(t.get<unsigned_type>()));
		}  else if (t.isFloating()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(t.get<floating_type>()));
			else
				s.append(toWString(t.get<floating_type>()));
		} else 
			s.append("null");
	}
	static constexpr void stringifyObject(const json_type& t, string_type& s) {
		s.pushBack('{');
		for (auto it = t.begin(); it != t.end(); it++) {
			if (it != t.begin()) s.pushBack(',');
			stringifyPair(*dynamic_cast<json_type*>(it->get()), s);
		}
		s.pushBack('}');
	}
	static constexpr void stringifyArray(const json_type& t, string_type& s) {
		s.pushBack('[');
		const auto& array = t.get<array_type>();
		for (auto it = array.rbegin(); it != array.rend(); it++) {
			if (it != array.rbegin()) s.pushBack(',');
			if ((*it)->isString())
				s.append("\"").append((*it)->template get<string_type>()).pushBack('\"');
			else if ((*it)->isObject())
				stringifyObject(**it, s);
			else if ((*it)->isArray())
				stringifyArray(**it, s);	
			else
				stringifyPrimitive(**it, s);
		}
		s.pushBack(']');
	}
	static constexpr void stringifyPair(const json_type& t, string_type& s) {
		s.append("\"").append(t.m_name).append("\":");
		
		if (t.isString())
			s.append("\"").append(t.get<string_type>()).append("\"");
		else if (t.isObject())
			stringifyObject(t, s);
		else if (t.isArray())
			stringifyArray(t, s);	
		else
			stringifyPrimitive(t, s);
	}

	static constexpr void stringifyObjectPretty(size_type indent, const json_type& t, string_type& s) {
		indent++;
		s.append("{\n");
		for (auto it = t.begin(); it != t.end(); it++) {
			if (it != t.begin()) s.append(",\n");
			stringifyPairPretty(indent, *it->get(), s);
		}
		s.append("\n").append(--indent, '\t').pushBack('}');
	}
	static constexpr void stringifyArrayPretty(size_type indent, const json_type& t, string_type& s) {
		indent += 1;
		s.append("[\n");
		const auto& array = t.get<array_type>();
		for (auto it = array.begin(); it != array.end(); it++) {
			if (it != array.begin()) s.append(",\n");
			s.append(indent, '\t');

			if ((*it)->isString())
				s.append("\"").append((*it)->template get<string_type>()).pushBack('\"');
			else if ((*it)->isObject())
				stringifyObjectPretty(indent, **it, s);
			else if ((*it)->isArray())
				stringifyArrayPretty(indent, **it, s);	
			else
				stringifyPrimitive(**it, s);
		}
		s.append("\n").append(--indent, '\t').pushBack(']');
	}
	static constexpr void stringifyPairPretty(size_type indent, const json_type& t, string_type& s) {
		s.append(indent, '\t').append("\"").append(t.m_name).append("\": ");
		
		if (t.isString())
			s.append("\"").append(t.get<string_type>()).pushBack('\"');
		else if (t.isObject())
			stringifyObjectPretty(indent, t, s);
		else if (t.isArray())
			stringifyArrayPretty(indent, t, s);	
		else
			stringifyPrimitive(t, s);
	}
};

using Json = BasicJson<>;
using SharedJson = BasicJson<char, Vector, std::int64_t, std::uint64_t, double, UnorderedSparseMap, SharedPointer>;
using WJson = BasicJson<wchar_t>;
using WSharedJson = BasicJson<wchar_t, Vector, std::int64_t, std::uint64_t, double, UnorderedSparseMap, SharedPointer>;

} // namespace lsd
