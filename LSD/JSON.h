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
#include "UnorderedDenseSet.h"
#include "String.h"
#include "StringView.h"
#include "FromChars.h"

#include "Detail/JSON/Core.h"

#include <type_traits>
#include <variant>
#include <charconv>

namespace lsd {

template <
	std::integral Literal = char,
	template <class...> class ArrayContainer = Vector,
	std::signed_integral Signed = std::int64_t,
	std::unsigned_integral Unsigned = std::uint64_t,
	std::floating_point Floating = double,
	template <class...> class NodeContainer = UnorderedDenseSet> 
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
	using view_type = BasicStringView<literal_type>;

	using key_type = string_type;
	using key_reference = key_type&;
	using const_key_reference = const key_type&;

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
	CUSTOM_HASHER(Hasher, const BasicJson&, const view_type&, Hash<view_type>{}, .m_name)
	CUSTOM_EQUAL(Equal, const BasicJson&, const view_type&, .m_name)

public:
	
	using container = NodeContainer<json_type, Hasher, Equal>;

	using iterator = typename container::iterator;
	using const_iterator = typename container::const_iterator;
	using iterator_pair = std::pair<iterator, bool>;

	constexpr BasicJson() noexcept : m_value(object_type { }) { }
	explicit constexpr BasicJson(const value_type& value) : m_value(value) { }
	explicit constexpr BasicJson(value_type&& value) : m_value(std::move(value)) { }
	template <class Value> explicit constexpr BasicJson(Value&& value) {
		assign(std::forward<Value>(value));
	}
	template <class KeyType> constexpr BasicJson(KeyType&& key, value_type&& value) : 
		m_name(std::forward<KeyType>(key)), m_value(std::move(value)) { }
	template <class KeyType, class Value> constexpr BasicJson(KeyType&& key, Value&& value) : m_name(std::forward<KeyType>(key)) {
		assign(std::forward<Value>(value));
	}
	constexpr BasicJson(const BasicJson& other) :
		m_value(other.m_value),
		m_name(other.m_name) {
		for (const auto& child : other.m_children) insert(child);
	}
	constexpr BasicJson(BasicJson&& other) : 
		m_value(std::move(other.m_value)),
		m_name(std::move(other.m_name)),
		m_children(std::move(other.m_children)) {
		for (auto& child : m_children) child.m_parent = this;
	}

	constexpr reference operator=(const_reference other) {
		m_value = other.m_value;
		m_name = other.m_name;
		for (const auto& child : other.m_children) insert(child);
		return *this;
	}
	constexpr reference operator=(rvreference other) {
		m_value = std::move(other.m_value);
		m_name = std::move(other.m_name);
		m_children = std::move(other.m_children);
		for (auto& child : m_children) child.m_parent = this;
		return *this;
	}

	constexpr reference operator=(const value_type& value) {
		m_value = value;
		return *this;
	}
	constexpr reference operator=(value_type&& value) {
		m_value = std::move(value);
		return *this;
	}
	template <class Value> constexpr reference operator=(const Value& value) { 
		assign(value);
		return *this;
	}
	template <class Value> constexpr reference operator=(Value&& value) {
		assign(std::forward<Value>(value));
		return *this;
	}
	
	template <class Ty> constexpr void assign(Ty&& value) requires (
		std::is_floating_point_v<std::remove_cvref_t<Ty>> ||
		std::is_unsigned_v<std::remove_cvref_t<Ty>> ||
		std::is_signed_v<std::remove_cvref_t<Ty>> ||
		std::is_constructible_v<string_type, Ty> ||
		std::is_constructible_v<value_type, Ty>
	) {
		if constexpr (std::is_floating_point_v<std::remove_cvref_t<Ty>>) m_value = static_cast<floating_type>(value);
		else if constexpr (std::is_unsigned_v<std::remove_cvref_t<Ty>>) m_value = static_cast<unsigned_type>(value);
		else if constexpr (std::is_signed_v<std::remove_cvref_t<Ty>>) m_value = static_cast<signed_type>(value);
		else if constexpr (std::is_constructible_v<string_type, Ty>) m_value.template emplace<string_type>(value);
		else m_value = std::forward<Ty>(value);
	}
	template <class Ty, class KeyType> constexpr void assign(KeyType&& key, const Ty& value) {
		assign(value);
		rename(std::forward<KeyType>(key));
	}
	template <class Ty, class KeyType> constexpr void assign(KeyType&& key, Ty&& value) {
		assign(value);
		rename(std::forward<KeyType>(key));
	}

	constexpr void swap(reference other) { 
		m_children.swap(other.m_children); 

		for (const auto& child : other.m_children)
			child.m_parent = &other;
		
		for (const auto& child : m_children)
			child.m_parent = this;

		m_name.swap(other.m_name);
		std::swap(m_parent, other.m_parent);
		std::swap(m_value, other.m_value);
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

	constexpr reference insert(const_reference child) {
		auto& res = *m_children.emplace(child).first;
		res.parent = this;

		return res;
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

	template <lsd::ContinuousIteratorType Iterator> [[nodiscard]] static constexpr json_type parse(Iterator begin, Iterator end) {
		// First node
		json_type json;
		if (begin == end) return json;

		skipCharacters(begin, end);

		// Start parsing
		if (*begin == '{') json.m_value = parseObject(begin, end, json);
		else if (*begin == '[') json.m_value = parseArray(begin, end);
		else if (++begin == end) json.m_value = object_type();
		else throw JsonParseError("lsd::Json::parse(): JSON Syntax Error: Unexpected symbol, JSON file has to either contain a single object or array at global scope or be empty!");

		return json;
	}
	template <lsd::IteratableContainer Container> [[nodiscard]] static constexpr json_type parse(const Container& container) {
		return parse(std::begin(container), std::end(container));
	}
	template <class CStringLike> [[nodiscard]] static constexpr json_type parse(CStringLike string) requires(
		(std::is_pointer_v<CStringLike>) &&
		std::is_integral_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_all_extents_t<std::remove_cvref_t<CStringLike>>>>>
	) {
		auto end = string;
		while (*end != '\0')
			++end;

		return parse(string, end);
	}

	constexpr string_type stringify() const {
		string_type r;
		auto formatHelper = detail::StringifyFormatHelper<string_type>();

		if (isObject()) stringifyObject(r, formatHelper);
		else if (isArray()) stringifyArray(r, formatHelper);
		else stringifyPair(r, formatHelper);

		return r;
	}
	constexpr string_type stringifyPretty() const {
		string_type r;
		auto formatHelper = detail::PrettyStringifyFormatHelper<string_type>();

		if (isObject()) stringifyObject(r, formatHelper);
		else if (isArray()) stringifyArray(r, formatHelper);
		else stringifyPair(r, formatHelper);

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
	constexpr auto& signedInt() noexcept {
		return std::get<signed_type>(m_value);
	}
	constexpr auto& unsignedInt() noexcept {
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

	template <class Ty> constexpr decltype(auto) get() const noexcept requires (
		std::is_same_v<std::remove_cvref_t<Ty>, json_type> ||
		std::is_floating_point_v<std::remove_cvref_t<Ty>> ||
		std::is_unsigned_v<std::remove_cvref_t<Ty>> ||
		std::is_signed_v<std::remove_cvref_t<Ty>> ||
		std::is_nothrow_convertible_v<literal_type*, Ty> ||
		std::is_nothrow_convertible_v<Ty, value_type>
	) {
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
		view_type k(key);
		size_type beg = 0, cur;
		const_pointer p = this;

		while ((cur = k.find("::", beg)) < k.size()) {
			p = &p->m_children.at(k.substr(beg, cur - beg));
			(beg = cur) += 2;
		}

		return p->m_children.at(k.substr(beg));
	}
	template <class KeyType> constexpr reference child(KeyType&& key) {
		view_type k(key);
		size_type beg = 0, cur;
		const_pointer p = this;

		while ((cur = k.find("::", beg)) < k.size()) {
			p = &p->m_children.at(k.substr(beg, cur - beg));
			beg = cur + 2;
		}

		return p->m_children.at(k.substr(beg));
	}

	const_reference at(size_type i) const {
		return array().at(i);
	}
	reference at(size_type i) {
		return array().at(i);
	}
	const_reference operator[](size_type i) const {
		return array()[i];
	}
	reference operator[](size_type i) {
		return array()[i];
	}

	template <class KeyType> constexpr const_reference at(KeyType&& name) const {
		return m_children.at(std::forward<KeyType>(name));
	}
	template <class KeyType> constexpr reference operator[](KeyType&& name) {
		return m_children[std::forward<KeyType>(name)];
	}


	[[nodiscard]] constexpr bool empty() const noexcept { 
		return m_children.empty(); 
	}
	explicit constexpr operator bool() const noexcept { 
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
		switch (*begin) {
			case ' ': case '\f': case '\n': case '\r': case '\t': case '\v': case '\0':
				for (++begin; begin != end; begin++) {
					switch (*begin) {
						case ' ': case '\f': case '\n': case '\r': case '\t': case '\v': case '\0':
							continue;
					}

					break;
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
						
						case 'u': {
							++begin;

							unsigned_type num = 0;
							std::size_t digitCount = 0;

							if (auto res = fromChars(begin, begin + 4, num, &digitCount, 16); res.ec != std::errc { } || digitCount != 4)
								throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, expected escaped hex character!");
							
							r.pushBack(static_cast<literal_type>(num));
							begin += 3;

							break;
						}
						
						default:
							r.pushBack(*begin);
					}

					break;

				case '\"':
					return r;
				
				default:
					r.pushBack(*begin);
			}
		}

		throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Missing symbol, string not terminated!");
		return r;
	}
	template <class Iterator> static constexpr value_type parsePrimitive(Iterator& begin, Iterator& end) {
		switch(*begin) {
			case 't':
				if (end - begin < 4 || !strncmp(++begin, "rue", 3))
					break;

				begin += 2;
				return true;

			case 'f':
				if (end - begin < 5 || !strncmp(++begin, "alse", 4))
					break;
				
				begin += 3;
				return false;
				
			case 'n':
				if (end - begin < 4 || !strncmp(++begin, "ull", 3))
					break;

				begin += 2;
				return value_type(null_type { });
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': {
				unsigned_type uRes { };
				if (auto res = fromChars(begin, end, uRes); 
					res.ec == std::errc { } && 
					*res.ptr != '.' && *res.ptr != 'e' && *res.ptr != 'p'
				) {
					begin = res.ptr - 1;
					return uRes;
				}

				break;
			}
			
			case '-':
				signed_type sRes = { };
				if (auto res = fromChars(begin, end, sRes); 
					res.ec == std::errc { } && 
					*res.ptr != '.' && *res.ptr != 'e' && *res.ptr != 'p'
				) {
					begin = res.ptr - 1;
					return sRes;
				}
		}

		floating_type fRes = { };
		if (auto res = fromChars(begin, end, fRes); res.ec == std::errc { }) {
			begin = res.ptr - 1;
			return fRes;
		}

		throw JsonParseError("lsd::Json::parsePrimitive(): JSON Syntax Error: Unexpected symbol, couldn't match identifier with any type!");
		return value_type();
	}

	template <class Iterator> static constexpr object_type parseObject(Iterator& begin, Iterator& end, json_type& json) {
		for (++begin; begin != end; begin++) {
			switch(skipCharacters(begin, end)) {
				case '}':
					return object_type();

				default:
					json.insert(parsePair(begin, end));

				case ',':
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

				default:
					tok.m_value = parsePrimitive(begin, end);
					r.emplaceBack(std::move(tok));

				case '}':
				case ',':
			}
		}

		throw JsonParseError("lsd::Json::parseArray(): JSON Syntax Error: Missing symbol!");

		return r;
	}
	template <class Iterator> static constexpr json_type parsePair(Iterator& begin, Iterator& end) {
		json_type tok;

		if (*begin != '\"')
			throw JsonParseError("lsd::Json::parseString(): JSON Syntax Error: Unexpected symbol, expected quotation marks!"); // The check is done here and not in the string because this is the only case where the validity of begin is not guaranteed
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
		}

		return tok;
	}


	// Stringification implementations

	constexpr void stringifyPrimitive(string_type& s) const {
		if (isBoolean()) {
			if (get<bool>() == true) s.append("true");
			else s.append("false");
		} else if (isSigned()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(get<signed_type>()));
			else
				s.append(toWString(get<signed_type>()));
		} else if (isUnsigned()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(get<unsigned_type>()));
			else
				s.append(toWString(get<unsigned_type>()));
		} else if (isFloating()) {
			if constexpr (sizeof(literal_type) == 1)
				s.append(toString(get<floating_type>()));
			else
				s.append(toWString(get<floating_type>()));
		} else 
			s.append("null");
	}
	constexpr void stringifyObject(string_type& s, auto& formatHelper) const {
		formatHelper.beginObject(s);
		
		for (auto it = begin(); it != end(); it++) {
			formatHelper.seperator(it != begin(), s);
			
			it->stringifyPair(s, formatHelper);
		}

		formatHelper.endObject(s);
	}
	constexpr void stringifyArray(string_type& s, auto& formatHelper) const {
		formatHelper.beginArray(s);

		const auto& array = get<array_type>();
		for (auto it = array.begin(); it != array.end(); it++) {
			formatHelper.seperator(it != array.begin(), s);
			
			if (it->isString())
				s.append("\"").append(it->template get<string_type>()).pushBack('\"');
			else if (it->isObject())
				it->stringifyObject(s, formatHelper);
			else if (it->isArray())
				it->stringifyArray(s, formatHelper);	
			else
				it->stringifyPrimitive(s);
		}
		
		formatHelper.endArray(s);
	}
	constexpr void stringifyPair(string_type& s, auto& formatHelper) const {
		(s += '\"').append(m_name).append("\": ");
		
		if (isString())
			(s += '\"').append(get<string_type>()).pushBack('\"');
		else if (isObject())
			stringifyObject(s, formatHelper);
		else if (isArray())
			stringifyArray(s, formatHelper);	
		else
			stringifyPrimitive(s);
	}


	friend struct HashFunction;
	friend struct EqualFunction;
};

using Json = BasicJson<>;
using WJson = BasicJson<wchar_t>;

} // namespace lsd
