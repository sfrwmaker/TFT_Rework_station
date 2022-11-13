/*
 * jsoncfg.h
 *
 */

#ifndef JSONCFG_H_
#define JSONCFG_H_

#include <vector>
#include <stack>
#include "JsonParser.h"
#include "ff.h"
#include "nls.h"

//--------------------------------------------------- Configuration file parser -------------------------------
class FILE_PARSER: public JsonListener {
	public:
    	FILE_PARSER() : JsonListener()              		{ }
    	virtual			~FILE_PARSER(void)					{ }
    	virtual void 	key(std::string key)                { d_key = key; }
    	virtual void	endObject();
    	virtual void	startObject();
    	virtual void	startArray();
    	virtual void	endArray();
    	virtual void	startDocument();
    	virtual void	endDocument()						{ }
    	virtual void	whitespace(char c)					{ }
	protected:
    	void			readFile(FIL *file);
    	std::string				d_key;
    	std::stack<std::string>	s_array;
    	std::stack<std::string> s_key;						// Json structure stack
//    	std::string				d_parent;
//    	std::string				d_array;
//    	std::stack<std::string> p_key;						// Json structure stack
};

//--------------------------------------------------- "cfg.json" main NLS configuration file parser -----------
typedef struct s_lang_cfg {
	std::string		lang;
	std::string		messages_file;
	std::string		font_file;
} t_lang_cfg;

typedef std::vector<t_lang_cfg> t_lang_list;

class JSON_LANG_CFG : public FILE_PARSER {
	public:
		JSON_LANG_CFG()                                		{ }
    	virtual void		endDocument();
		virtual void 		value(std::string value);
		void				readConfig(FIL *file);
		void				addEnglish();					// Add default (English) language entry to the list
		uint8_t				listSize(void)					{ return lang_list.size();	}
		t_lang_list			*getLangList(void)				{ return &lang_list; 		}
	private:
		t_lang_cfg			data;
		t_lang_list 		lang_list;						// Use vector to save language list config
};

//--------------------------------------------------- Messages parser -----------------------------------------
class JSON_MESSAGES : public FILE_PARSER {
	public:
		JSON_MESSAGES()                                		{ }
		void				readConfig(FIL *file)			{ readFile(file);		}
		void				setNLS_MSG(NLS_MSG *pMsg)		{ this->pMsg = pMsg;	}
		virtual void 		value(std::string value);
	private:
		NLS_MSG				*pMsg		= 0;
};

#endif
