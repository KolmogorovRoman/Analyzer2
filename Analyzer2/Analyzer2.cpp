#include <iostream>
#include <vector>
#include <list>
#include <optional>
#include <ranges>
#include <memory>
#include <string>
#include <map>
#include <ranges>
#include <set>
#include <regex>
#include <variant>
//import std;
//import std.regex;
//import Tokenizer;
#include "Tokenizer.h";
//export module Analyzer;

using namespace std::string_literals;

//struct Symbol
//{
//	bool isTerminal;
//	std::optional<Token::Type> token_type;
//	std::string name;
//	Symbol(std::string name):
//		isTerminal(false),
//		name(name)
//	{}
//	Symbol(Token::Type token_type):
//		isTerminal(true),
//		token_type(token_type),
//		name(token_type.name)
//	{}
//};
struct Terminal
{
	Token::Type token_type;
	Terminal(Token::Type token_type):
		token_type(token_type)
	{}
};
struct NonTerminal
{
	std::string name;
	NonTerminal(std::string name):
		name(name)
	{}
};
struct Symbol: std::variant<nullptr_t, const Terminal*, const NonTerminal*>
{
	std::string name;
	Symbol(nullptr_t):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(nullptr),
		name(""s)
	{}Symbol(const Terminal* terminal):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(terminal),
		name(terminal->token_type.name)
	{}
	Symbol(const NonTerminal* non_terminal):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(non_terminal),
		name(non_terminal->name)
	{}
	bool isTerminal() const
	{
		return std::holds_alternative<const Terminal*>(*this);
	}
	Symbol* operator->()
	{
		return this;
	}
	template <class T> bool is() const
	{
		return std::holds_alternative<T>(*this);
	}
	template <class T> T as() const
	{
		if (!this->is<T>()) throw "Invalid cast";
		return std::get<T>(*this);
	}
};
//struct Symbol: std::variant<Terminal, NonTerminal>
//{
//	std::string name;
//	Symbol(Terminal terminal):
//		std::variant<Terminal, NonTerminal>(terminal)
//	{}
//	Symbol(std::string name):
//		std::variant<Terminal, NonTerminal>(NonTerminal(name))
//	{}
//	Symbol(NonTerminal non_terminal):
//		std::variant<Terminal, NonTerminal>(non_terminal)
//	{}
//	Symbol(Token::Type token_type):
//		std::variant<Terminal, NonTerminal>(Terminal(token_type))
//	{}
//	bool isTerminal() const
//	{
//		return std::holds_alternative<Terminal>(*this);
//	}
//};

struct Rule
{
	Symbol left;
	std::vector<Symbol> right;
	std::string view;
	Rule(Symbol left, const std::vector<Symbol>& right, const std::string& view):
		left(left),
		right(right),
		view(view)
	{}
	Rule(const NonTerminal* left, const std::vector<Symbol>& right):
		left(left),
		right(right)
	{
		view = left->name + "->"s;
		if (right.size() == 0) view += "_";
		else for (Symbol s : right)
		{
			view += s.name + " ";
		}
	}
	/*Rule(const std::vector<const Symbol*>& start):
		left(nullptr),
		right(start)
	{
		view = ""s;
		for (const Symbol* s : start)
		{
			view += s->name;
		}
		right.push_back(nullptr);
	}
	Rule(const Symbol* terminal):
		left(terminal),
		right()
	{
		view = "["s + terminal->name + "]"s;
	}*/
	static Rule* makeStart(const NonTerminal* start, const Terminal* end)
	{
		return new Rule(nullptr, { start, end, nullptr }, start->name);
	}
	//static Rule* makeStart(const std::vector<const Symbol*>& start)
	//{
	//	std::string view = ""s;
	//	for (const Symbol* s : start)
	//	{
	//		view += s->name;
	//	}
	//	return new Rule(nullptr, start, view);
	//}
	static Rule* makeChecking(const Terminal* terminal)
	{
		return new Rule(terminal, {}, "["s + terminal->token_type.name + "]"s);
	}

};

struct TreeNode
{
	TreeNode* parent;
	const Rule* rule;
	std::list<std::unique_ptr<TreeNode>> childs;
	TreeNode() = default;
	TreeNode(const Rule* rule):
		rule(rule),
		parent(nullptr),
		childs()
	{}
	void out(int offset) const
	{
		std::cout << std::string(offset, ' ') << rule->view << std::endl;
		for (const std::unique_ptr<TreeNode>& c : childs)
		{
			c->out(offset + 1);
		}
	}
};

struct State: std::enable_shared_from_this<State>
{
	struct SymbolPointer
	{
		const State* state;
		std::vector<Symbol>::const_iterator iterator;
		SymbolPointer(const State* state, std::vector<Symbol>::const_iterator iterator):
			state(state),
			iterator(iterator)
		{}
		SymbolPointer(std::nullptr_t = nullptr):
			state(nullptr)
		{}
		Symbol getSymbol() const
		{
			return *iterator;
		}
		Symbol operator->() const
		{
			return *iterator;
		}
		Symbol operator*() const
		{
			return *iterator;
		}
	};

	const Rule* rule;
	const std::shared_ptr<const State> prev;
	SymbolPointer parent;
	std::list<Symbol>::iterator symbol;
	SymbolPointer front_symbol;
	Tokenizer tokenizer;
	struct Cycle
	{
		std::shared_ptr<State> state;
		std::list<Symbol>::iterator begin;
		//std::list<const Symbol*>::iterator end;
		Cycle(std::shared_ptr<State> state, std::list<Symbol>::iterator begin/*, std::list<const Symbol*>::iterator end*/):
			state(state),
			begin(begin)
			//end(end)
		{}
		Cycle(std::shared_ptr<State> state):
			state(state)
		{}
	};
	std::list<Cycle> cycles;

	State(const Rule* rule, std::shared_ptr<const State> prev, SymbolPointer parent):
		rule(rule),
		prev(prev),
		parent(parent),
		front_symbol(this, std::begin(rule->right)),
		tokenizer(prev->tokenizer)
	{
		while (front_symbol.iterator == std::end(front_symbol.state->rule->right))
		{
			if (front_symbol.state == nullptr) throw "Stack is empty";
			front_symbol = SymbolPointer(front_symbol.state->parent.state, std::next(front_symbol.state->parent.iterator));
		}
	}
	State(const Rule* start_rule, const std::string& code):
		rule(start_rule),
		prev(nullptr),
		parent(nullptr),
		front_symbol(this, std::begin(start_rule->right)),
		tokenizer(code)
	{}
	bool ruleApplicable(const Rule* rule) const
	{
		return rule->left == *front_symbol;
	}
	std::shared_ptr<State> advanced(const Rule* rule) const
	{
		if (!ruleApplicable(rule)) throw "Invalid source stack";
		return std::make_shared<State>(rule, shared_from_this(), front_symbol);
	}
	std::unique_ptr<TreeNode> getTree() const
	{
		std::list<const State*> statesList;
		const State* state = this;
		while (state != nullptr)
		{
			statesList.push_front(state);
			state = state->prev.get();
		}

		const State* parent = nullptr;
		//statesList.pop_front();
		std::unique_ptr<TreeNode> overRoot = std::make_unique<TreeNode>();
		TreeNode* parentNode = overRoot.get();
		for (const State* state : statesList)
		{
			while (state->parent.state != parent)
			{
				parent = parent->parent.state;
				parentNode = parentNode->parent;
			}
			std::unique_ptr<TreeNode> node = std::make_unique<TreeNode>(state->rule);
			node->parent = parentNode;
			parentNode->childs.push_back(std::move(node));
			parent = state;
			parentNode = parentNode->childs.back().get();
		}
		std::unique_ptr<TreeNode> root = std::move(overRoot->childs.front());
		root->parent = nullptr;
		return root;
	}
};

class Analyzer
{
	std::map<std::string, Symbol> symbolbyName;

	const Rule* start_rule;
	const Terminal* end_symbol;
	std::map<const NonTerminal*, std::list<const Rule*>> expandingRules;
	std::map<const Terminal*, Rule*> checkingRules;
	std::list<std::shared_ptr<State>> expand(const std::shared_ptr<State>& leaf)
	{
		std::list<std::shared_ptr<State>> leafs({ leaf });
		//std::set<const Symbol*> expandedSymbols;
		std::list<std::shared_ptr<State>> cycles;
		for (std::list<std::shared_ptr<State>>::iterator state = std::begin(leafs); state != std::end(leafs);)
		{
			//std::shared_ptr<State> leaf = *state;
			if (state->get()->front_symbol->isTerminal())
			{
				state++;
				continue;
			}
			/*for (const State* l = state->get()->parent.state; l != leaf->parent.state; l = l->parent.state)
			{
				if (state->get()->front_symbol.getSymbol() == l->front_symbol.getSymbol())
				{
					stateRepeats = true;
					break;
				}
			}*/
			else
			{
				//expandedSymbols.insert(*(*state)->front_symbol);
				/*std::ranges::view auto leafs_to_insert =
					std::views::transform(expandingRules[*leaf->front_symbol],
										  [&](const Rule* rule) { return leaf->advanced(rule); });*/
				std::shared_ptr<State> current_state = *state;
				state = leafs.erase(state);
				for (const Rule* rule : expandingRules[current_state->front_symbol.getSymbol().as<const NonTerminal*>()])
				{
					std::shared_ptr<State> new_state = current_state->advanced(rule);
					bool stateRepeats = false;
					for (const State* l = current_state.get(); l != leaf->parent.state; l = l->parent.state)
					{
						if (new_state->front_symbol.getSymbol() == l->front_symbol.getSymbol())
						{
							stateRepeats = true;
							break;
						}
					}
					if (stateRepeats)
					{
						cycles.push_back(new_state);
					}
					else
					{
						state = leafs.insert(state, new_state);
					}
				}
				//state = leafs.insert(state, leafs_to_insert.begin(), leafs_to_insert.end());
			}
		}
		for (const std::shared_ptr<State>& leaf : leafs)
		{
			for (const std::shared_ptr<State>& c : cycles)
				leaf->cycles.push_back(State::Cycle(c));
		}
		return leafs;
	}
public:
	Analyzer(std::list<Symbol> symbols, const NonTerminal* start, std::list<const Rule*> rules)
	{
		for (const Symbol& symbol : symbols)
		{
			if (symbol.is<const Terminal*>())
				checkingRules[symbol.as<const Terminal*>()] = Rule::makeChecking(symbol.as<const Terminal*>());
		}
		end_symbol = new Terminal(Token::Type::eof);
		checkingRules[end_symbol] = Rule::makeChecking(end_symbol);
		start_rule = Rule::makeStart(start, end_symbol);
		for (const Rule* rule : rules)
		{
			expandingRules[rule->left.as<const NonTerminal*>()].push_back(rule);
		}
	}
	/*Analyzer(const std::list<std::string>& terminals,
			 const std::list<std::string>& nonterminals,
			 const std::string& start,
			 const std::list<std::list<std::string>>& rules)
	{
		for (const std::string& terminal : terminals)
		{
			symbolbyName[terminal] = new Symbol(true, terminal);
			checkingRules[symbolbyName[terminal]] = Rule::makeChecking(symbolbyName[terminal]);
		}
		for (const std::string& nonterminal : nonterminals)
		{
			symbolbyName[nonterminal] = new Symbol(false, nonterminal);
		}
		end_symbol = new Symbol(true, "eof");
		symbolbyName["eof"s] = end_symbol;
		checkingRules[end_symbol] = Rule::makeChecking(end_symbol);
		start_rule = Rule::makeStart(symbolbyName[start], end_symbol);
		for (std::list<std::string> rule : rules)
		{
			const Symbol* left = symbolbyName[rule.front()];
			rule.pop_front();
			std::vector<const Symbol*> right;
			for (const std::string& r : rule)
				right.push_back(symbolbyName[r]);
			expandingRules[left].push_back(new Rule(left, right));
		}
	}*/
	/*std::list<std::shared_ptr<State>> analyze(std::list<const Symbol*>::iterator begin, std::list<const Symbol*>::iterator end)
	{
		std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule) };
		for (std::list<const Symbol*>::iterator i = begin; i != end; i = std::next(i))
			for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
				for (const std::shared_ptr<State>& expanded_leaf : expand(*leaf))
					if (expanded_leaf->ruleApplicable(checkingRules[*i]))
					{
						leafs.insert(leaf, expanded_leaf->advanced(checkingRules[*i]));
						for (State::Cycle& cycle : expanded_leaf->cycles)
						{
							cycle.begin = i;
						}
					}

		return leafs;
	}*/
	//std::unique_ptr<TreeNode> analyze(std::list<const Symbol*> input, std::string code)
	//{
	//	input.push_back(end_symbol);
	//	std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule, code) };
	//	for (std::list<const Symbol*>::iterator i = input.begin(); i != input.end(); i = std::next(i))
	//		for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
	//			for (const std::shared_ptr<State>& expanded_leaf : expand(*leaf))
	//				if (expanded_leaf->ruleApplicable(checkingRules[*i]))
	//				{
	//					leafs.insert(leaf, expanded_leaf->advanced(checkingRules[*i]));
	//					for (State::Cycle& cycle : expanded_leaf->cycles)
	//					{
	//						cycle.begin = i;
	//					}
	//				}
	//	if (leafs.size() == 0) throw "Code is invalid";
	//	else if (leafs.size() > 1) throw "Code is ambiguous";
	//	else return leafs.front()->getTree();
	//	//return analyze(input.begin(), input.end()).front()->getTree();
	//}
	std::unique_ptr<TreeNode> analyze(std::string code)
	{
		std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule, code) };
		std::list<std::shared_ptr<State>> end_leafs;
		while (!leafs.empty())
			for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
				for (const std::shared_ptr<State>& expanded_leaf : expand(*leaf))
				{
					const Terminal* front_symbol = expanded_leaf->front_symbol->as<const Terminal*>();
					std::optional<Token> token = expanded_leaf->tokenizer.getToken(front_symbol->token_type);
					if (token.has_value())
					{
						if (front_symbol == end_symbol)
							end_leafs.push_back(expanded_leaf->advanced(checkingRules[front_symbol]));
						else leafs.insert(leaf, expanded_leaf->advanced(checkingRules[front_symbol]));
					}
				}
		if (end_leafs.size() == 0) throw "Code is invalid";
		else if (end_leafs.size() > 1) throw "Code is ambiguous";
		else return end_leafs.front()->getTree();
		//return analyze(input.begin(), input.end()).front()->getTree();
	}
	/*std::unique_ptr<TreeNode> analyze(std::list<std::string> text_input, std::string code)
	{
		std::list<const Symbol*> input;
		for (const std::string& i : text_input)
			input.push_back(symbolbyName[i]);
		return analyze(input, code);
	}*/
};

int main()
{
	/*Token::Type ta("a"s, std::regex("a"));
	Token::Type tb("b"s, std::regex("b"));

	const Terminal* sa = new Terminal(ta);
	const Terminal* sb = new Terminal(tb);
	const NonTerminal* sA = new NonTerminal("A"s);
	const NonTerminal* sB = new NonTerminal("B"s);

	std::list<const Rule*> rules
	{	new Rule(sA, { sa, sB }),
		new Rule(sB, { }),
		new Rule(sA, { }),
		new Rule(sB, { sb, sA }),
	};
	Analyzer a({ sa,sb,sA,sB }, sA, rules);
	std::unique_ptr<TreeNode> root = a.analyze("ababababab"s);
	root->out(0);*/

	Token::Type variable("variable", std::regex("[[:alpha:]][[:alnum:]]*"));
	Token::Type integer("integer", std::regex("[[:digit:]]+"));
	Token::Type oper_plus("+", std::regex("\\+"));
	Token::Type oper_mul("*", std::regex("\\*"));
	Token::Type oper_eq("=", std::regex("="));
	Token::Type oper_lb("(", std::regex("\\("));
	Token::Type oper_rb(")", std::regex("\\)"));

	const Terminal* s_variable = new Terminal(variable);
	const Terminal* s_integer = new Terminal(integer);
	const Terminal* s_oper_mul = new Terminal(oper_mul);
	const Terminal* s_oper_plus = new Terminal(oper_plus);
	const Terminal* s_oper_eq = new Terminal(oper_eq);
	const Terminal* s_oper_lb = new Terminal(oper_lb);
	const Terminal* s_oper_rb = new Terminal(oper_rb);
	const NonTerminal* s_expression = new NonTerminal("expression"s);
	const NonTerminal* s_val = new NonTerminal("val"s);
	const NonTerminal* s_sum = new NonTerminal("sum"s);
	const NonTerminal* s_mul = new NonTerminal("mul"s);

	std::list<const Rule*> rules
	{	new Rule(s_expression, { s_sum }),
		new Rule(s_sum, { s_mul }),
		new Rule(s_sum, { s_mul, s_oper_plus, s_sum }),
		new Rule(s_val, { s_oper_lb, s_sum, s_oper_rb }),
		new Rule(s_mul, { s_val }),
		new Rule(s_mul, { s_val, s_oper_mul, s_mul }),
		new Rule(s_val, { s_variable }),
		new Rule(s_val, { s_integer }),
	};
	Analyzer a({ s_variable,s_integer,s_oper_mul,s_oper_plus,s_oper_eq,s_oper_lb,s_oper_rb,s_expression,s_val,s_sum,s_mul }, s_expression, rules);
	std::unique_ptr<TreeNode> root = a.analyze("qwe*(abc+42)+34"s);
	root->out(0);

	/*std::unique_ptr<TreeNode> root = Analyzer(
		{ "a", "b", },
		{ "A", "B", },
		"A",
		{
			{"A", "a", "B"},
			{"B"},
			{"A"},
			{"B", "b", "A"},
		}
	).analyze({ "a", "b", "a" });
	root->out(0);

	root = Analyzer(
		{ "a", "b", "c" },
		{ "A", "B", "C" },
		"A",
		{
			{"A", "B", "C"},
			{"B"},
			{"B", "b", "B"},
			{"C", "c"},
		}
	).analyze({ "b", "b", "c" });
	root->out(0);*/

	/*root = Analyzer(
		{ "a", "b" },
		{ "A", "B" },
		"A",
		{
			{"A", "B", "a"},
			{"B", "A", "b"},
			{"A"},
			{"B"},
		}
	).analyze({ "a", "b", "a" });
	root->out(0);*/

	/*std::string code = "qwe1=abc+42"s;
	Tokenizer T(code);
	Token::Type variable("variable", std::regex("[[:alpha:]][[:alnum:]]*"));
	Token::Type oper("oper", std::regex("[+=]+"));
	Token::Type integer("integer", std::regex("[[:digit:]]+"));

	std::optional<Token> t;
	t = T.getToken(variable);
	t = T.getToken(oper);
	t = T.getToken(variable);
	t = T.getToken(oper);
	t = T.getToken(integer);*/
	return 0;
}