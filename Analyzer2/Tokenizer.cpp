#include "Tokenizer.h"
//import std;
//import std.regex;
//export module Tokenizer;

Token::Type::Type(std::string name, std::regex regex):
	name(name), regex(regex)
{}
Token::Token(Type type, std::string value):
	type(type), value(value)
{}

Token::Type Token::Type::eof("eof", std::regex("$"));

Tokenizer::Tokenizer(const std::string& code):
	code(code),
	head(this->code.cbegin())
{}
std::optional<Token> Tokenizer::getToken(Token::Type type)
{
	std::match_results<std::string::const_iterator> m;
	std::regex_search(head, code.cend(), m, type.regex, std::regex_constants::match_continuous);
	if (m.empty()) return std::optional<Token>();
	else
	{
		head += m.length();
		return Token(type, m.str());
	}
}