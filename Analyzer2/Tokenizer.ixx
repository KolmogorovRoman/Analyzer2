#include <string>
#include <optional>
#include <regex>
export module Tokenizer;

export struct Token;
export class Tokenizer;

struct Token
{
	struct Type
	{
		std::string name;
		std::regex regex;
		Type(std::string name, std::regex regex):
			name(name), regex(regex)
		{}
	};
	Type type;
	std::string value;
	Token(Type type, std::string value):
		type(type), value(value)
	{}
};

class Tokenizer
{
	std::string text;
	std::string::const_iterator head;
public:
	Tokenizer(std::string text):
		text(text),
		head(this->text.cbegin())
	{}
	std::optional<Token> getToken(Token::Type type)
	{
		std::match_results<std::string::const_iterator> m;
		std::regex_search(head, text.cend(), m, type.regex, std::regex_constants::match_continuous);
		if (m.empty()) return std::optional<Token>();
		else
		{
			head += m.length();
			return Token(type, m.str());
		}
	}
};