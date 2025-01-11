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

#include "Vector.h"
#include "UnorderedSparseSet.h"
#include "String.h"
#include "FromChars.h"

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
	template <class...> class NodeContainer = UnorderedSparseSet> 
class BasicJson {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using null_type = JsonNull;
	using object_type = JsonObject;
	using signed_type = Signed;
	using unsigned_type = Unsigned;
	using floating_type = Floating;
	using literal_type = Literal;
	using string_type = BasicString<literal_type>;

	using key_type = string_type;
	using key_reference = key_type&;
	using const_key_reference = const key_type&;
	using key_rvreference = key_type&&;

	using json_type = BasicJson;
	using pointer = json_type*;
	using const_pointer = const json_type*;
	using reference = json_type&;
	using const_reference = const json_type&;
	using rvreference = json_type&&;

	using array_type = ArrayContainer<json_type>;
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

private:
	CUSTOM_HASHER(Hasher, const BasicJson&, const_key_reference, Hash<key_type>{}, .m_name)
	CUSTOM_EQUAL(Equal, const BasicJson&, const_key_reference, .m_name)

public:
	
	using container = NodeContainer<json_type, Hasher, Equal>;

	using iterator = typename container::iterator;
	using const_iterator = typename container::const_iterator;
	using iterator_pair = std::pair<iterator, bool>;

	constexpr BasicJson() noexcept : m_value(object_type { }) { }
	constexpr BasicJson(const value_type& value) noexcept : m_value(value) { }
	constexpr BasicJson(value_type&& value) noexcept : m_value(std::move(value)) { }
	template <class KeyType> 
	constexpr BasicJson(KeyType&& key, value_type&& value) noexcept : m_name(std::forward<KeyType>(key)), m_value(std::move(value)) { }
	template <class KeyType, class Value> 
	constexpr BasicJson(KeyType&& key, Value&& value) noexcept : m_name(std::forward<KeyType>(key)) {
		assign(std::forward<Value>(value));
	}
	constexpr BasicJson(const BasicJson&) = default;
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

	constexpr void swap(reference other) noexcept { 
		m_children.swap(other.m_children); 
	}


	constexpr iterator begin() noexcept {
		return m_children.begin();
	}
	constexpr const_iterator begin() const noexcept {
		return m_children.begin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return m_children.cbegin();
	}

	constexpr iterator end() noexcept {
		return m_children.end();
	}
	constexpr const_iterator end() const noexcept {
		return m_children.end();
	}
	constexpr const_iterator cend() const noexcept {
		return m_children.cend();
	}

	constexpr reference insert(rvreference child) {
		child.m_parent = this;
		auto& res = *m_children.emplace(std::move(child)).first;

		return res;
	}
	template <class... Args> constexpr reference emplace(Args&&... args) {
		auto& res = *m_children.emplace(std::forward<Args>(args)...).first;
		res.m_parent = this;

		return res;
	}


	constexpr reference erase(iterator pos) { 
		return m_children.erase(pos); 
		return *this;
	}
	constexpr reference erase(const_iterator pos) { 
		return m_children.erase(pos); 
		return *this;
	}
	constexpr reference erase(const_iterator first, const_iterator last) { 
		return m_children.erase(first, last); 
		return *this;
	}
	template <class KeyType> constexpr size_type erase(KeyType&& name) requires std::is_convertible_v<KeyType, key_type> { 
		return m_children.erase(std::forward(name)); 
	}

	constexpr reference clear() noexcept { 
		m_children.clear();
		return *this;
	}

	template <lsd::IteratorType Iterator> [[nodiscard]] static constexpr json_type parse(Iterator begin, Iterator end) {
		// first node
		json_type json;

		if (*begin == '#') { // Fast json mode, doesn't skip whitespaces since there shouldn't be any and doesn't check for validness
			++begin;
			if (begin == end) json.m_value = object_type();
			else if (*begin == '{') json.m_value = parseObjectFast(begin, end, json);
			else if (*begin == '[') json.m_value = parseArrayFast(begin, end);
		} else {
			skipCharacters(begin, end);

			// start parsing
			if (*begin == '{') json.m_value = parseObject(begin, end, json);
			else if (*begin == '[') json.m_value = parseArray(begin, end);
			else if (++begin == end) json.m_value = object_type();
			else throw JsonParseError("lsd::Json::parse(): JSON Syntax Error: Unexpected symbol, JSON file has to either contain a single object or array at global scope or be empty!");
		}

		return json;
	}
	template <lsd::IteratableContainer Container> [[nodiscard]] static constexpr json_type parse(const Container& container) {
		return parse(std::begin(container), std::end(container));
	}
	template <class CStringLike> [[nodiscard]] static constexpr json_type parse(const CStringLike& string) requires(
		(std::is_pointer_v<CStringLike>) &&
		std::is_integral_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_all_extents_t<std::remove_cvref_t<CStringLike>>>>>
	) {
		auto end = string;
		while (*end != '\0')
			++end;

		return parse(string, end);
	}

	constexpr string_type stringify(bool fastJson = false) const {
		string_type r;
		if (fastJson) r.pushBack('#');

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


	template <class KeyType> constexpr iterator find(KeyType&& name) { 
		return m_children.find(std::forward<KeyType>(name)); 
	}
	template <class KeyType> constexpr const_iterator find(KeyType&& name) const { 
		return m_children.find(std::forward<KeyType>(name)); 
	}
	template <class KeyType> constexpr bool contains(KeyType&& name) const { 
		return m_children.contains(std::forward<KeyType>(name)); 
	}

	template <class KeyType> constexpr const_reference child(KeyType&& key) const {
		constexpr bool stringlike = requires(const KeyType& k) {
			key_type(key);
			key_type().find("");
			key_type().substr(0, 0);
		};

		if constexpr (stringlike) {
			key_type k(key);
			size_type beg = 0, cur;
			const_pointer p = this;

			while ((cur = k.find("::", beg)) < k.size()) {
				p = &p->m_children.at(k.substr(beg, cur - beg));
				(beg = cur) += 2;
			}

			return *p->m_children.at(k.substr(beg)).get();
		} else {
			return *m_children.at(key).get();
		}
	}
	template <class KeyType> constexpr reference child(KeyType&& key) {
		constexpr bool stringlike = requires(const KeyType& k) {
			key_type(key);
			key_type().find("");
			key_type().substr(0, 0);
		};

		if constexpr (stringlike) {
			key_type k(key);
			size_type beg = 0, cur;
			const_pointer p = this;

			while ((cur = k.find("::", beg)) < k.size()) {
				p = p->m_children.at(k.substr(beg, cur - beg));
				beg = cur + 2;
			}

			return *p->m_children.at(k.substr(beg)).get();
		} else {
			return *m_children.at(key).get();
		}
	}

	const_reference at(size_type i) const {
		return *get<array_type>().at(i);
	}
	reference operator[](size_type i) {
		return *get<array_type>()[i];
	}

	template <class KeyType> constexpr const_reference at(KeyType&& name) const {
		*m_children.at(std::forward<KeyType>(name));
	}
	template <class KeyType> constexpr reference at(KeyType&& name) {
		*m_children.at(std::forward<KeyType>(name));
	}
	
	template <class KeyType> constexpr const_reference operator[](KeyType&& name) const {
		return *m_children[std::forward<KeyType>(name)];
	}
	template <class KeyType> constexpr reference operator[](KeyType&& name) {
		return *m_children[std::forward<KeyType>(name)];
	}


	[[nodiscard]] constexpr bool empty() const noexcept { 
		return m_children.empty(); 
	}
	constexpr operator bool() const noexcept { 
		return m_children.empty(); 
	}

	[[nodiscard]] constexpr size_type size() const noexcept {
		return m_children.size();
	}
	[[nodiscard]] constexpr const_key_reference name() const noexcept {
		return m_name;
	}
	[[nodiscard]] constexpr const_pointer const parent() const noexcept {
		return m_parent;
	}

private:
	value_type m_value;

	key_type m_name { };

	pointer m_parent = nullptr;
	container m_children { };


	// Parsing functions

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
		string_type r;

		for (++begin; begin != end; begin++) {
			switch (*begin) {
				case '\\':
					++begin;

					if (begin == end) {
						throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Missing symbol, string not terminated!");

						return r;
					}

					switch (*begin) {
						case 'b':
							r.pushBack('\b');

							break;

						case 't':
							r.pushBack('\t');
							
							break;

						case 'n':
							r.pushBack('\n');
							
							break;

						case 'f':
							r.pushBack('\f');
							
							break;

						case 'r':
							r.pushBack('\r');
							
							break;
						
						case 'u':
							/// @todo 4 hex digits

							break;

						default:
							r.pushBack(*begin);
					}

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
					auto sequenceBegin = begin;

					for (; begin != end; begin++) {
						switch (*(begin + 1)) { 
							case ' ':
							case '\f':
							case '\n':
							case '\r':
							case '\t':
							case '\v':
							case '\0': 
							case '}':
							case ']':
							case ',':
								break;
							
							default:
								continue;
						}

						break;
					}

					auto sequenceEnd = begin + 1;

					unsigned_type uRes { };

					if (fromChars(sequenceBegin, sequenceEnd, uRes).ptr == sequenceEnd) return uRes;
					else {
						signed_type sRes = { };

						if (fromChars(sequenceBegin, sequenceEnd, sRes).ptr == sequenceEnd) return sRes;
						else {
							floating_type fRes = { };
							
							if (fromChars(sequenceBegin, sequenceEnd, fRes).ptr == sequenceEnd) return fRes;
							else throw JsonParseError("lsd::Json::parsePrimitive(): JSON Syntax Error: Unexpected symbol, number could not be parsed!");
						}
					}
				}

				break;
			
			default:
				throw JsonParseError("lsd::Json::parsePrimitive(): JSON Syntax Error: Unexpected symbol, couldn't match identifier with any type!");
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
			json_type tok { };

			switch(skipCharacters(begin, end)) {
				case '{':
					tok.m_value = parseObject(begin, end, tok);
					r.emplaceBack(std::move(tok));

					break;

				case '[':
					tok.m_value = parseArray(begin, end);
					r.emplaceBack(std::move(tok));

					break;

				case '\"':
					tok.m_value = parseString(begin, end);
					r.emplaceBack(std::move(tok));

					if (*(begin + 1) != ']') ++begin;

					break;

				case ']':
					return r;
					break;

				case '}':
				case ',':
					break;

				default:
					tok.m_value = parsePrimitive(begin, end);
					r.emplaceBack(std::move(tok));
					
					break;
			}
		}

		throw JsonParseError("lsd::Json::parseArray(): JSON Syntax Error: Missing symbol!");

		return r;
	}
	template <class Iterator> static constexpr json_type parsePair(Iterator& begin, Iterator& end) {
		json_type tok;

		if (*begin != '\"')
			throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, expected quotation marks!"); // the check is done here and not in the string because this is the only case where the validity of begin is not guaranteed
		tok.m_name = parseString(begin, end);

		if (++begin == end)
			throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol!");
		if (skipCharacters(begin, end) != ':')
			throw JsonParseError("lsd::Json::parsePair(): JSON Syntax Error: Unexpected symbol, expected double colon after variable name!");
		if (++begin == end)
			throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol!");

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

	template <class Iterator> static constexpr object_type parseObjectFast(Iterator& begin, Iterator& end, json_type& json) {
		for (++begin; begin != end; begin++) {
			switch(*begin) {
				default:
					json.insert(parsePairFast(begin, end));
					break;
					
				case '}':
					return object_type();
					break;
				case ',':
					break;
			}
		}

		return object_type();
	}
	template <class Iterator> static constexpr array_type parseArrayFast(Iterator& begin, Iterator& end) {
		array_type r;

		for (++begin; begin != end; begin++) {
			json_type tok { };

			switch(*begin) {
				case '{':
					tok.m_value = parseObjectFast(begin, end, tok);
					r.emplaceBack(std::move(tok));

					break;

				case '[':
					tok.m_value = parseArrayFast(begin, end);
					r.emplaceBack(std::move(tok));

					break;

				case '\"':
					tok.m_value = parseString(begin, end);
					r.emplaceBack(std::move(tok));

					if (*(begin + 1) != ']') ++begin;

					break;

				case ']':
					return r;
					break;

				case '}':
				case ',':
					break;

				default:
					tok.m_value = parsePrimitive(begin, end);
					r.emplaceBack(std::move(tok));
					
					break;
			}
		}

		return r;
	}
	template <class Iterator> static constexpr json_type parsePairFast(Iterator& begin, Iterator& end) {
		json_type tok;

		tok.m_name = parseString(begin, end);
		++begin; // skip '"'
		++begin; // skip ':'

		switch(*begin) {
			case '{':
				tok.m_value = parseObjectFast(begin, end, tok);

				break;
			case '[':
				tok.m_value = parseArrayFast(begin, end);

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


	// Stringification implementations

	static constexpr void stringifyPrimitive(const json_type& t, string_type& s) {
		if (t.isBoolean()) {
			if (t.get<bool>() == true) s.append("true");
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
			stringifyPair(*it, s);
		}
		s.pushBack('}');
	}
	static constexpr void stringifyArray(const json_type& t, string_type& s) {
		s.pushBack('[');
		const auto& array = t.get<array_type>();
		for (auto it = array.rbegin(); it != array.rend(); it++) {
			if (it != array.rbegin()) s.pushBack(',');
			if (it->isString())
				s.append("\"").append(it->template get<string_type>()).pushBack('\"');
			else if (it->isObject())
				stringifyObject(*it, s);
			else if (it->isArray())
				stringifyArray(*it, s);	
			else
				stringifyPrimitive(*it, s);
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
			stringifyPairPretty(indent, *it, s);
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

			if (it->isString())
				s.append("\"").append(it->template get<string_type>()).pushBack('\"');
			else if (it->isObject())
				stringifyObjectPretty(indent, *it, s);
			else if (it->isArray())
				stringifyArrayPretty(indent, *it, s);	
			else
				stringifyPrimitive(*it, s);
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


	friend struct HashFunction;
	friend struct EqualFunction;
};

using Json = BasicJson<>;
using WJson = BasicJson<wchar_t>;

} // namespace lsd
