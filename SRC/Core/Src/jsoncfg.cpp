/*
 * jsoncfg.cpp
 *
 */

#include "jsoncfg.h"
#include "vars.h"

//--------------------------------------------------- Configuration file parser -------------------------------
/*
void FILE_PARSER::startDocument() {
	while (!p_key.empty() ) {
		p_key.pop();
	}
	d_key = d_parent = "";
}

void FILE_PARSER::startObject() {
	p_key.push(d_parent);
	d_parent = d_key;
}

void FILE_PARSER::endObject() {
	d_parent = p_key.top();
	p_key.pop();
	d_key.clear();
}

void FILE_PARSER::startArray() {
	p_key.push(d_parent);
	d_parent = d_key;
	d_array	 = d_key;
}

void FILE_PARSER::endArray() {
	d_parent = p_key.top();
	p_key.pop();
	d_key.clear();
	d_array	= d_parent;
}
*/

void FILE_PARSER::startDocument() {
	while (!s_key.empty() ) {
		s_key.pop();
	}
	d_key.clear();
}

void FILE_PARSER::startObject() {
	s_key.push(d_key);
}

void FILE_PARSER::endObject() {
	s_key.pop();
	d_key.clear();
}

void FILE_PARSER::startArray() {
	s_array.push(d_key);
}

void FILE_PARSER::endArray() {
	s_array.pop();
	d_key.clear();
}

void FILE_PARSER::readFile(FIL *file) {
	JsonStreamingParser parser;
	parser.setListener(this);

	bool is_body = false;
	while(true) {
		UINT	br = 0;										// Number of bytes actually read from the file
		uint8_t c;											// Byte to be read
		f_read(file, (void *)&c, 1, &br);
		if (br == 1) {										// Byte read successfully
			if (!is_body && (c == '{' || c == '[')) {
				is_body = true;
			}
			if (is_body) {
				parser.parse(c);
			}
		} else {											// end of file reached
			f_close(file);
			return;
		}
	}
}

//--------------------------------------------------- "cfg.json" main NLS configuration file parser -----------
void JSON_LANG_CFG::readConfig(FIL *file) {
	while (!lang_list.empty() ) {							// First, clear lang_list
		lang_list.pop_back();
	}
	readFile(file);
}

/*
 * Language configuration file contains the language list in the following format.
 * Each entry has name (a string to be displayed in the main menu)
 * messages file contains the message configuration data
 * font file contains u8g2 font to be loaded to draw the messages
 * {
	"languages": [
		{ "name": "russian", "messages": "ru_lang.json", "font": "ru.font"},
		{ "name": "french",  "messages": "fr_lang.json", "font": "fr.font"}
	]
}
 */
void JSON_LANG_CFG::value(std::string value) {
	std::string array = s_array.top();
	if (array.compare("languages") == 0) {
		if (d_key.compare("name") == 0) {					// Found new language entry
			if (!data.lang.empty() && data.lang.compare(value) != 0) {
				lang_list.push_back(data);					// Save previous language data to the language list if the language is different
			}
			data.lang = value;								// Initialize next language data structure
			data.font_file.clear();
			data.messages_file.clear();
		} else if (d_key.compare("messages") == 0) {
			data.messages_file = value;
		} else if (d_key.compare("font") == 0) {
			data.font_file = value;
		}
	}
}

// Commit last language
void JSON_LANG_CFG::endDocument() {
	if (!data.lang.empty() && !data.messages_file.empty()) { // Language can use default font. In this case, font entry will be empty
		lang_list.push_back(data);
	}
}

// Add default language (English) to the language list
void JSON_LANG_CFG::addEnglish() {
	data.lang = std::string(def_language);					// "English"
	data.font_file.clear();									// Use default font
	data.messages_file.clear();								// Use default messages
	t_lang_list::const_iterator first = lang_list.begin();
	lang_list.insert(first, data);
}

//--------------------------------------------------- Messages parser -----------------------------------------
void JSON_MESSAGES::value(std::string value) {
	if (pMsg) {
		pMsg->set(d_key, value, s_key.top());
	}
}
