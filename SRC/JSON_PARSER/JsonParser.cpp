/**The MIT License (MIT)

JsonParser.cpp

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

#include "JsonParser.h"

JsonStreamingParser::JsonStreamingParser(void) {
    reset();
}

void JsonStreamingParser::reset() {
    state = STATE_START_DOCUMENT;
    buffer_pos					= 0;
    unicode_escape_buffer_pos	= 0;
    unicode_buffer_pos			= 0;
    character_counter			= 0;
}

void JsonStreamingParser::setListener(JsonListener* listener) {
	myListener = listener;
}

void JsonStreamingParser::parse(char c) {
	if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
        && !(state == STATE_IN_STRING || state == STATE_UNICODE || state == STATE_START_ESCAPE
            || state == STATE_IN_NUMBER || state == STATE_START_DOCUMENT)) {
		return;
    }
    switch (state) {
    case STATE_IN_STRING:
    	if (c == '"') {
    		endString();
    	} else if (c == '\\') {
    		state = STATE_START_ESCAPE;
    	} else if ((c < 0x1f) || (c == 0x7f)) {
    		//throw new RuntimeException("Unescaped control character encountered: " + c + " at position" + characterCounter);
    	} else {
    		buffer[buffer_pos] = c;
    		increaseBufferPointer();
    	}
    	break;
    case STATE_IN_ARRAY:
    	if (c == ']') {
    		endArray();
    	} else {
    		startValue(c);
    	}
    	break;
    case STATE_IN_OBJECT:
    	if (c == '}') {
    		endObject();
    	} else if (c == '"') {
    		startKey();
    	} else {
    		//throw new RuntimeException("Start of string expected for object key. Instead got: " + c + " at position" + characterCounter);
    	}
    	break;
    case STATE_END_KEY:
    	if (c != ':') {
    		// Try to Fix error
    		if (c == '"') {
    			startValue(c);
    			break;		// startValue() sets stat to STATE_IN_STRING, Do not change the state
    		}
    	}
    	state = STATE_AFTER_KEY;
    	break;
    case STATE_AFTER_KEY:
    	startValue(c);
    	break;
    case STATE_START_ESCAPE:
    	processEscapeCharacters(c);
    	break;
    case STATE_UNICODE:
    	processUnicodeCharacter(c);
    	break;
    case STATE_UNICODE_SURROGATE:
    	unicode_escape_buffer[unicode_escape_buffer_pos] = c;
    	unicode_escape_buffer_pos++;
    	if (unicode_escape_buffer_pos == 2) {
    		endUnicodeSurrogateInterstitial();
    	}
    	break;
    case STATE_AFTER_VALUE: {
    	// not safe for size == 0!!!
    	int within = stack[stackPos - 1];
    	if (within == STACK_OBJECT) {
    		if (c == '}') {
    			endObject();
    		} else if (c == ',') {
    			state = STATE_IN_OBJECT;
    		} else {
    			//throw new RuntimeException("Expected ',' or '}' while parsing object. Got: " + c + ". " + characterCounter);
    		}
    	} else if (within == STACK_ARRAY) {
    		if (c == ']') {
    			endArray();
    		} else if (c == ',') {
    			state = STATE_IN_ARRAY;
    		} else {
    			//throw new RuntimeException("Expected ',' or ']' while parsing array. Got: " + c + ". " + characterCounter);
    		}
    	} else {
    		//throw new RuntimeException("Finished a literal, but unclear what state to move to. Last state: " + characterCounter);
    	}
    }
    	break;
    case STATE_IN_NUMBER:
    	if (c >= '0' && c <= '9') {
    		buffer[buffer_pos] = c;
    		increaseBufferPointer();
    	} else if (c == '.') {
    		if (doesCharArrayContain(buffer, buffer_pos, '.')) {
    			//throw new RuntimeException("Cannot have multiple decimal points in a number. " + characterCounter);
    		} else if (doesCharArrayContain(buffer, buffer_pos, 'e')) {
    			//throw new RuntimeException("Cannot have a decimal point in an exponent." + characterCounter);
    		}
    		buffer[buffer_pos] = c;
    		increaseBufferPointer();
    	} else if (c == 'e' || c == 'E') {
    		if (doesCharArrayContain(buffer, buffer_pos, 'e')) {
    			//throw new RuntimeException("Cannot have multiple exponents in a number. " + characterCounter);
    		}
    		buffer[buffer_pos] = c;
    		increaseBufferPointer();
    	} else if (c == '+' || c == '-') {
    		char last = buffer[buffer_pos - 1];
    		if (!(last == 'e' || last == 'E')) {
    			// throw new RuntimeException("Can only have '+' or '-' after the 'e' or 'E' in a number." + characterCounter);
    		}
    		buffer[buffer_pos] = c;
    		increaseBufferPointer();
    	} else {
    		endNumber();
    		// we have consumed one beyond the end of the number
    		parse(c);
    	}
    	break;
    case STATE_IN_TRUE:
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    	if (buffer_pos == 4) {
    		endTrue();
    	}
    	break;
    case STATE_IN_FALSE:
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    	if (buffer_pos == 5) {
    		endFalse();
    	}
    	break;
    case STATE_IN_NULL:
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    	if (buffer_pos == 4) {
    		endNull();
    	}
    	break;
    case STATE_START_DOCUMENT:
    	myListener->startDocument();
    	if (c == '[') {
    		startArray();
    	} else if (c == '{') {
    		startObject();
    	} else {
    		// throw new ParsingError($this->_line_number,
    		// $this->_char_number,
    		// "Document must start with object or array.");
    	}
      break;
    case STATE_DONE:
    	break;
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected end of document.");
    default:
    	break;
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Internal error. Reached an unknown state: ".$this->_state);
    }
    character_counter++;
}

void JsonStreamingParser::increaseBufferPointer() {
	if (buffer_pos < JSON_BUFFER_MAX_LENGTH - 2)
		++buffer_pos;
}

void JsonStreamingParser::endString() {
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped == STACK_KEY) {
    	buffer[buffer_pos] = '\0';
    	std::string k(buffer);
    	myListener->key(k);
    	state = STATE_END_KEY;
    } else if (popped == STACK_STRING) {
    	buffer[buffer_pos] = '\0';
    	myListener->value(std::string(buffer));
    	state = STATE_AFTER_VALUE;
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Unexpected end of string.");
    }
    buffer_pos = 0;
}

void JsonStreamingParser::startValue(char c) {
    if (c == '[') {
    	startArray();
    } else if (c == '{') {
    	startObject();
    } else if (c == '"') {
    	startString();
    } else if (isDigit(c)) {
    	startNumber(c);
    } else if (c == 't') {
    	state = STATE_IN_TRUE;
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    } else if (c == 'f') {
    	state = STATE_IN_FALSE;
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    } else if (c == 'n') {
    	state = STATE_IN_NULL;
    	buffer[buffer_pos] = c;
    	increaseBufferPointer();
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Unexpected character for value: ".$c);
    }
}

bool JsonStreamingParser::isDigit(char c) {
    // Only concerned with the first character in a number.
    return (c >= '0' && c <= '9') || c == '-';
  }

void JsonStreamingParser::endArray() {
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped != STACK_ARRAY) {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Unexpected end of array encountered.");
    }
    myListener->endArray();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
    	endDocument();
    }
}

void JsonStreamingParser::startKey() {
    stack[stackPos] = STACK_KEY;
    stackPos++;
    state = STATE_IN_STRING;
}

void JsonStreamingParser::endObject() {
    int popped = stack[stackPos];
    stackPos--;
    if (popped != STACK_OBJECT) {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Unexpected end of object encountered.");
    }
    myListener->endObject();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
    	endDocument();
    }
}

void JsonStreamingParser::processEscapeCharacters(char c) {
    if (c == '"') {
    	buffer[buffer_pos] = '"';
    	increaseBufferPointer();
    } else if (c == '\\') {
    	buffer[buffer_pos] = '\\';
    	increaseBufferPointer();
    } else if (c == '/') {
    	buffer[buffer_pos] = '/';
    	increaseBufferPointer();
    } else if (c == 'b') {
    	buffer[buffer_pos] = 0x08;
    	increaseBufferPointer();
    } else if (c == 'f') {
    	buffer[buffer_pos] = '\f';
    	increaseBufferPointer();
    } else if (c == 'n') {
    	buffer[buffer_pos] = '\n';
    	increaseBufferPointer();
    } else if (c == 'r') {
    	buffer[buffer_pos] = '\r';
    	increaseBufferPointer();
    } else if (c == 't') {
    	buffer[buffer_pos] = '\t';
    	increaseBufferPointer();
    } else if (c == 'u') {
    	state = STATE_UNICODE;
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected escaped character after backslash. Got: ".$c);
    }
    if (state != STATE_UNICODE) {
    	state = STATE_IN_STRING;
    }
}

void JsonStreamingParser::processUnicodeCharacter(char c) {
    if (!isHexCharacter(c)) {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected hex character for escaped Unicode character. Unicode parsed: "
    	// . implode($this->_unicode_buffer) . " and got: ".$c);
    }

    unicode_buffer[unicode_buffer_pos] = c;
    unicode_buffer_pos++;

    if (unicode_buffer_pos == 4) {
    	int codepoint = getHexArrayAsDecimal(unicode_buffer, unicode_buffer_pos);
    	endUnicodeCharacter(codepoint);
    	return;
    	/*if (codepoint >= 0xD800 && codepoint < 0xDC00) {
        	unicodeHighSurrogate = codepoint;
        	unicodeBufferPos = 0;
        	state = STATE_UNICODE_SURROGATE;
      	  } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
        	if (unicodeHighSurrogate == -1) {
          	  // throw new ParsingError($this->_line_number,
          	  // $this->_char_number,
          	  // "Missing high surrogate for Unicode low surrogate.");
        	}
        	int combinedCodePoint = ((unicodeHighSurrogate - 0xD800) * 0x400) + (codepoint - 0xDC00) + 0x10000;
        	endUnicodeCharacter(combinedCodePoint);
      	  } else if (unicodeHighSurrogate != -1) {
        	// throw new ParsingError($this->_line_number,
        	// $this->_char_number,
        	// "Invalid low surrogate following Unicode high surrogate.");
        	endUnicodeCharacter(codepoint);
      	  } else {
        	endUnicodeCharacter(codepoint);
      	  }*/
    }
}

bool JsonStreamingParser::isHexCharacter(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int JsonStreamingParser::getHexArrayAsDecimal(char hexArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
    	char current = hexArray[length - i - 1];
    	int value = 0;
    	if (current >= 'a' && current <= 'f') {
    		value = current - 'a' + 10;
    	} else if (current >= 'A' && current <= 'F') {
    		value = current - 'A' + 10;
    	} else if (current >= '0' && current <= '9') {
    		value = current - '0';
    	}
    	result += value * 16^i;
    }
    return result;
}

bool JsonStreamingParser::doesCharArrayContain(char myArray[], int length, char c) {
    for (int i = 0; i < length; i++) {
    	if (myArray[i] == c) {
    		return true;
    	}
    }
    return false;
}

void JsonStreamingParser::endUnicodeSurrogateInterstitial() {
    char unicodeEscape = unicode_escape_buffer[unicode_escape_buffer_pos - 1];
    if (unicodeEscape != 'u') {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected '\\u' following a Unicode high surrogate. Got: " .
    	// $unicode_escape);
    }
    unicode_buffer_pos = 0;
    unicode_escape_buffer_pos = 0;
    state = STATE_UNICODE;
}

void JsonStreamingParser::endNumber() {
    buffer[buffer_pos] = '\0';
    std::string value = std::string(buffer);
    //float result = 0.0;
    //if (doesCharArrayContain(buffer, buffer_pos, '.')) {
    //  result = value.toFloat();
    //} else {
      // needed special treatment in php, maybe not in Java and c
    //  result = value.toFloat();
    //}
    myListener->value(value.c_str());
    buffer_pos = 0;
    state = STATE_AFTER_VALUE;
}

int JsonStreamingParser::convertDecimalBufferToInt(char myArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
    	char current = myArray[length - i - 1];
    	result += (current - '0') * 10;
    }
    return result;
}

void JsonStreamingParser::endDocument() {
    myListener->endDocument();
    state = STATE_DONE;
}

void JsonStreamingParser::endTrue() {
    buffer[buffer_pos] = '\0';
    std::string value = std::string(buffer);
    if (value.compare("true") == 0) {
    	myListener->value("true");
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected 'true'. Got: ".$true);
    }
    buffer_pos = 0;
    state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endFalse() {
    buffer[buffer_pos] = '\0';
    std::string value = std::string(buffer);
    if (value.compare("false") == 0) {
    	myListener->value("false");
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected 'true'. Got: ".$true);
    }
    buffer_pos = 0;
    state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endNull() {
    buffer[buffer_pos] = '\0';
    std::string value = std::string(buffer);
    if (value.compare("null") == 0) {
    	myListener->value("null");
    } else {
    	// throw new ParsingError($this->_line_number, $this->_char_number,
    	// "Expected 'true'. Got: ".$true);
    }
    buffer_pos = 0;
    state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::startArray() {
    myListener->startArray();
    state = STATE_IN_ARRAY;
    stack[stackPos] = STACK_ARRAY;
    stackPos++;
}

void JsonStreamingParser::startObject() {
    myListener->startObject();
    state = STATE_IN_OBJECT;
    stack[stackPos] = STACK_OBJECT;
    stackPos++;
}

void JsonStreamingParser::startString() {
    stack[stackPos] = STACK_STRING;
    stackPos++;
    state = STATE_IN_STRING;
}

void JsonStreamingParser::startNumber(char c) {
    state = STATE_IN_NUMBER;
    buffer[buffer_pos] = c;
    increaseBufferPointer();
}

void JsonStreamingParser::endUnicodeCharacter(int codepoint) {
    buffer[buffer_pos] = convertCodepointToCharacter(codepoint);
    increaseBufferPointer();
    unicode_buffer_pos = 0;
    unicode_high_surrogate = -1;
    state = STATE_IN_STRING;
}

char JsonStreamingParser::convertCodepointToCharacter(int num) {
    if (num <= 0x7F)
      return (char) (num);
    // if(num<=0x7FF) return (char)((num>>6)+192) + (char)((num&63)+128);
    // if(num<=0xFFFF) return
    // chr((num>>12)+224).chr(((num>>6)&63)+128).chr((num&63)+128);
    // if(num<=0x1FFFFF) return
    // chr((num>>18)+240).chr(((num>>12)&63)+128).chr(((num>>6)&63)+128).chr((num&63)+128);
    return ' ';
}
