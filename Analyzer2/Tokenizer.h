#pragma once
#include <regex>
#include <string>

struct Token
{
	struct Type
	{
		std::string dbg_name;
		std::regex regex;
		Type(std::string dbg_name, std::regex regex);
		static Token::Type eof;
	};
	Type type;
	std::string value;
	Token(Type type, std::string value);
};

class Tokenizer
{
public:
	const std::string* code;
	std::string::const_iterator head;
	Tokenizer(const std::string& code);
	Tokenizer(const std::string& code, std::string::const_iterator head);
	Tokenizer& operator=(const Tokenizer& other);
	bool checkToken(Token::Type type);
	std::optional<Token> getToken(Token::Type type) const;
	Tokenizer advanced(Token::Type type) const;
};