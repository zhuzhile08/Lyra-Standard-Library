#include <LSD/StringView.h>
#include <LSD/JSON.h>

#include <cstdio>

inline constexpr auto jsonTest = "{\
	\"string\"      :   \"A basic string\"  ,\
	\"emptyString\":   \"\"   ,\
	\"number\" :12345,\
	\"negativeNumber\"    :-12345 ,\
	\"float\"  :123.45 ,\
	\"negativeFloat\" :   -123.45,\
	\"zero\" :    0   ,\
	\"booleanTrue\": true   ,\
	\"booleanFalse\":false,\
	\"nullValue\"  :  null ,\
	\"emptyArray\" :   [  ] ,\
	\"arrayWithMixedValues\":    [\
		\"string\" ,\
		123 ,\
		true ,\
		null ,\
		{     \"nestedObject\":     \"value\"   }\
	] ,\
	\"nestedObjects\": {\
		\"level1\" : {   \
			\"level2\": {   \
				\"level3\":    { \
					\"key\"   :\"deepValue\"\
				}   \
			}    \
		}  \
	} ,\
	\"arrayOfObjects\" : [\
		{ \"id\" : 1 , \"value\":\"A\" },\
		{ \"id\": 2 , \"value\" : \"B\" },\
		{  \"id\"   :3 , \"value\":\"C\"   }\
	] ,\
	\"escapedCharacters\" :\"Quotes: \\\" Backslash: \\\\\\ Newline: \\\\n Tab: \\\\t\"  ,\
	\"unicodeCharacters\":    \"\\u0041\\u00E9\\u672C\",\
	\"emptyObject\": {  } ,\
	\"largeNumber\"  :1234567890123456789 ,  \
	\"longString\"    :  \"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\"  ,\
	\"booleanArray\" : [  true ,false, true  , false  ] ,\
	\"nestedArrays\"  : [\
		[1 ,  2,3] ,\
		[  \"a\"  ,\"b\"  , \"c\"   ],\
		[  [ null, true   ]  ,  false ]\
	]\
}";

int main() {
	lsd::Json json = lsd::Json::parse(jsonTest);
	std::printf("%s\n", json.stringifyPretty().data());
}
