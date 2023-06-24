#pragma once
#include <regex>
#include <string>

struct Token
{
	struct Type
	{
		std::string name;
		std::regex regex;
		Type(std::string name, std::regex regex);
		static Token::Type eof;
	};
	Type type;
	std::string value;
	Token(Type type, std::string value);
};

class Tokenizer
{
	const std::string& code;
	std::string::const_iterator head;
public:
	Tokenizer(const std::string& code);
	std::optional<Token> getToken(Token::Type type);
};