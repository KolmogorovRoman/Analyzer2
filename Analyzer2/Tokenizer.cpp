#include "Tokenizer.h"
//import std;
//import std.regex;
//export module Tokenizer;

Token::Type::Type(std::string name, std::regex regex):
	dbg_name(name), regex(regex)
{}
Token::Token(Type type, std::string value):
	type(type), value(value)
{}

Token::Type Token::Type::eof("eof", std::regex("$"));

Tokenizer::Tokenizer(const std::string& code):
	code(&code),
	head(this->code->cbegin())
{}
Tokenizer::Tokenizer(const std::string& code, std::string::const_iterator head):
	code(&code),
	head(head)
{}
Tokenizer& Tokenizer::operator=(const Tokenizer& other)
{
	this->code = other.code;
	this->head = other.head;
	return *this;
}
bool Tokenizer::checkToken(Token::Type type)
{
	std::match_results<std::string::const_iterator> m;
	std::regex_search(head, code->cend(), m, type.regex, std::regex_constants::match_continuous);
	return !m.empty();
}
std::optional<Token> Tokenizer::getToken(Token::Type type) const
{
	std::match_results<std::string::const_iterator> m;
	std::regex_search(head, code->cend(), m, type.regex, std::regex_constants::match_continuous);
	if (m.empty()) return std::optional<Token>();
	else return Token(type, m.str());
}
Tokenizer Tokenizer::advanced(Token::Type type) const
{
	std::match_results<std::string::const_iterator> m;
	std::regex_search(head, code->cend(), m, type.regex, std::regex_constants::match_continuous);
	return Tokenizer(*code, head + m.length());
}