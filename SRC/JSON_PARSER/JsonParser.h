/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch and https://github.com/squix78/json-streaming-parser
*/

#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include <string>

// Maximum string length in JSON configuration  file
#define JSON_BUFFER_MAX_LENGTH (40)

class JsonListener {
	public:
		JsonListener(void)								{ }
		virtual			~JsonListener(void)				{ }
		virtual void	whitespace(char c)				= 0;
		virtual void	startDocument()					= 0;
		virtual void	key(std::string key)			= 0;
		virtual void	value(std::string value)		= 0;
		virtual void	endArray()						= 0;
		virtual void	endObject()						= 0;
		virtual void	endDocument()					= 0;
		virtual void	startArray()					= 0;
		virtual void	startObject()					= 0;
};

enum e_state {
	STATE_START_DOCUMENT = 0, STATE_DONE, STATE_IN_ARRAY, STATE_IN_OBJECT, STATE_END_KEY,
	STATE_AFTER_KEY, STATE_IN_STRING, STATE_START_ESCAPE, STATE_UNICODE, STATE_IN_NUMBER,
	STATE_IN_TRUE, STATE_IN_FALSE, STATE_IN_NULL, STATE_AFTER_VALUE, STATE_UNICODE_SURROGATE
};

enum e_stack {
	STACK_OBJECT = 0, STACK_ARRAY, STACK_KEY, STACK_STRING
};

class JsonStreamingParser {
	public:
    	JsonStreamingParser(void);
    	void		parse(char c);
    	void		setListener(JsonListener* listener);
    	void		reset();
	private:
    	void		increaseBufferPointer();
    	void		endString();
    	void		endArray();
    	void		startValue(char c);
    	void		startKey();
    	void		processEscapeCharacters(char c);
    	bool		isDigit(char c);
    	bool		isHexCharacter(char c);
    	char		convertCodepointToCharacter(int num);
    	void		endUnicodeCharacter(int codepoint);
    	void		startNumber(char c);
    	void		startString();
    	void		startObject();
    	void		startArray();
    	void		endNull();
    	void		endFalse();
    	void		endTrue();
    	void		endDocument();
    	int			convertDecimalBufferToInt(char myArray[], int length);
    	void		endNumber();
    	void		endUnicodeSurrogateInterstitial();
    	bool		doesCharArrayContain(char myArray[], int length, char c);
    	int			getHexArrayAsDecimal(char hexArray[], int length);
    	void		processUnicodeCharacter(char c);
    	void		endObject();
    	enum e_state	state;
    	int 			stack[20];
    	int 			stackPos = 0;
    	JsonListener*	myListener;
    	bool			do_emit_whitespace = false;
    	char 			buffer[JSON_BUFFER_MAX_LENGTH];
    	int 			buffer_pos = 0;
    	char			unicode_escape_buffer[10];
    	uint8_t			unicode_escape_buffer_pos = 0;
    	char			unicode_buffer[10];
    	uint8_t			unicode_buffer_pos = 0;
    	int16_t			character_counter = 0;
    	int16_t			unicode_high_surrogate = 0;
};

#endif
