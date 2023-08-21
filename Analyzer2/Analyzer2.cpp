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
#include <functional>
//import std;
//import std.regex;
//import Tokenizer;
#include "Tokenizer.h";
//export module Analyzer;

using namespace std::string_literals;

struct State;
struct TreeNode;

//struct Symbol
//{
//	bool isTerminal;
//	std::optional<Token::Type> token_type;
//	std::string dbg_name;
//	Symbol(std::string dbg_name):
//		isTerminal(false),
//		dbg_name(dbg_name)
//	{}
//	Symbol(Token::Type token_type):
//		isTerminal(true),
//		token_type(token_type),
//		dbg_name(token_type.dbg_name)
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
	std::string dbg_name;
	NonTerminal(std::string dbg_name):
		dbg_name(dbg_name)
	{}
};
struct Symbol: std::variant<nullptr_t, const Terminal*, const NonTerminal*>
{
	std::string dbg_name;
	Symbol(nullptr_t):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(nullptr),
		dbg_name(""s)
	{}
	Symbol(const Terminal* terminal):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(terminal),
		dbg_name(terminal->token_type.dbg_name)
	{}
	Symbol(const NonTerminal* non_terminal):
		std::variant<nullptr_t, const Terminal*, const NonTerminal*>(non_terminal),
		dbg_name(non_terminal->dbg_name)
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
//	std::string dbg_name;
//	Symbol(Terminal terminal):
//		std::variant<Terminal, NonTerminal>(terminal)
//	{}
//	Symbol(std::string dbg_name):
//		std::variant<Terminal, NonTerminal>(NonTerminal(dbg_name))
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
	std::string dbg_view;
	Rule(Symbol left, const std::vector<Symbol>& right, const std::string& view):
		left(left),
		right(right),
		dbg_view(view)
	{}
	Rule(const NonTerminal* left, const std::vector<Symbol>& right):
		left(left),
		right(right)
	{
		dbg_view = left->dbg_name + "->"s;
		if (right.size() == 0) dbg_view += "_";
		else for (Symbol s : right)
		{
			dbg_view += s.dbg_name + " ";
		}
	}
	/*Rule(const std::vector<const Symbol*>& start):
		left(nullptr),
		right(start)
	{
		dbg_view = ""s;
		for (const Symbol* s : start)
		{
			dbg_view += s->dbg_name;
		}
		right.push_back(nullptr);
	}
	Rule(const Symbol* terminal):
		left(terminal),
		right()
	{
		dbg_view = "["s + terminal->dbg_name + "]"s;
	}*/
	static Rule* makeStart(const NonTerminal* start, const Terminal* end)
	{
		return new Rule(nullptr, { start, end, nullptr }, start->dbg_name);
	}
	//static Rule* makeStart(const std::vector<const Symbol*>& start)
	//{
	//	std::string dbg_view = ""s;
	//	for (const Symbol* s : start)
	//	{
	//		dbg_view += s->dbg_name;
	//	}
	//	return new Rule(nullptr, start, dbg_view);
	//}
	static Rule* makeChecking(const Terminal* terminal)
	{
		return new Rule(terminal, {}, "["s + terminal->token_type.dbg_name + "]"s);
	}

};

struct TreeNode
{
	TreeNode* parent;
	const Rule* rule;
	Tokenizer tokenizer;
	std::vector<std::unique_ptr<TreeNode>> childs;
	//TreeNode();
	TreeNode(const State* state);
	void out(int offset) const;
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
	std::list<const State*> dbg_hist;
	std::list<std::string> dbg_stack;
	int dbg_depth;
	struct Cycle
	{
		std::shared_ptr<State> state;
		std::shared_ptr<State> begin;
		//bool passed = false;
		friend bool operator == (const Cycle& c1, const Cycle& c2)
		{
			return c1.state == c2.state && c1.begin == c2.begin;
		}
		Cycle(std::shared_ptr<State> state, std::shared_ptr<State> begin):
			state(state),
			begin(begin)
		{}
		Cycle(std::shared_ptr<State> state):
			state(state)
		{}
	};
	//std::list<Cycle> repeated_states;
	std::map<const Rule*, Cycle> cycles;

	State(const Rule* rule, std::shared_ptr<const State> prev, SymbolPointer parent):
		rule(rule),
		prev(prev),
		parent(parent),
		front_symbol(this, std::begin(rule->right)),
		tokenizer(prev->tokenizer)
	{
		dbg_hist = prev->dbg_hist;
		dbg_hist.push_back(prev.get());
		dbg_stack = parent.state->dbg_stack;
		dbg_stack.pop_front();
		std::list<std::string>::iterator b = dbg_stack.begin();
		for (const Symbol& s : rule->right)
			dbg_stack.insert(b, s.dbg_name);
		dbg_depth = parent.state->dbg_depth + 1;

		if (rule->left.isTerminal()) tokenizer = prev->tokenizer.advanced(rule->left.as<const Terminal*>()->token_type);

		//if (prev->repeated_states.size() > 0)
		cycles = prev->cycles;
	/*for (const Cycle& c : prev->repeated_states)
		if (!c.passed)
		{
			repeated_states.push_back(c);
		}*/
		while (front_symbol.iterator == std::end(front_symbol.state->rule->right))
		{
			if (front_symbol.state == nullptr) throw "Stack is empty";
			front_symbol = SymbolPointer(front_symbol.state->parent.state, std::next(front_symbol.state->parent.iterator));
			for (std::map<const Rule*, Cycle>::iterator c = std::begin(cycles); c != std::end(cycles);)
				if (c->second.begin.get() == front_symbol.state)
					c = cycles.erase(c);
				else
					c = std::next(c);
		}
		/*for (Cycle& c : repeated_states)
			if (c.will_be_passed)
				c.passed = true;*/
	}
	State(const Rule* start_rule, const std::string& code):
		rule(start_rule),
		prev(nullptr),
		parent(nullptr),
		front_symbol(this, std::begin(start_rule->right)),
		tokenizer(code),
		dbg_hist(),
		dbg_stack({ start_rule->right[0].dbg_name, start_rule->right[1].dbg_name }),
		dbg_depth(0)
	{}
	bool isParentOf(const State& mbChild) const
	{
		for (const State* s = &mbChild; s != nullptr; s = s->parent.state)
		{
			if (s == this) return true;
		}
		return false;
	}
	bool isChildOf(const State& mbParent) const
	{
		return mbParent.isParentOf(*this);
	}
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
		std::unique_ptr<TreeNode> overRoot = std::make_unique<TreeNode>(statesList.front());
		TreeNode* parentNode = overRoot.get();
		for (const State* state : statesList)
		{
			while (state->parent.state != parent)
			{
				parent = parent->parent.state;
				parentNode = parentNode->parent;
			}
			std::unique_ptr<TreeNode> node = std::make_unique<TreeNode>(state);
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

//TreeNode::TreeNode() = default;
TreeNode::TreeNode(const State* state):
	rule(state->rule),
	tokenizer(state->tokenizer),
	parent(nullptr),
	childs()
{}
void TreeNode::out(int offset) const
{
	std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
	for (const std::unique_ptr<TreeNode>& c : childs)
	{
		c->out(offset + 1);
	}
}

class Analyzer
{
	std::map<std::string, Symbol> symbolbyName;

	const Terminal* end_symbol;
	const Rule* start_rule;
	std::map<const NonTerminal*, std::list<const Rule*>> expandingRules;
	std::map<const Terminal*, Rule*> checkingRules;
	std::list<std::shared_ptr<State>> expand(const std::shared_ptr<State>& leaf)
	{
		std::list<std::shared_ptr<State>> leafs({ leaf });
		//std::set<const Symbol*> expandedSymbols;
		std::list<std::shared_ptr<State>> repeated_states;
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
				/*std::ranges::dbg_view auto leafs_to_insert =
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
						if (new_state->rule == l->rule)
						{
							stateRepeats = true;
							break;
						}
					}
					if (stateRepeats)
					{
						repeated_states.push_back(new_state);
					}
					else
					{
						state = leafs.insert(state, new_state);
					}
				}
				//state = leafs.insert(state, leafs_to_insert.begin(), leafs_to_insert.end());
			}
		}
		for (const std::shared_ptr<State>& l : leafs)
		{
			for (const std::shared_ptr<State>& c : repeated_states)
				//l->repeated_states[c->rule] = State::Cycle(c, leaf);
				//l->cycles.insert_or_assign(c->rule, State::Cycle(c, leaf));
			{
				bool cycle_added = l->cycles.insert(std::make_pair(c->rule, State::Cycle(c, leaf))).second;
				if (!cycle_added) l->cycles.at(c->rule).state = c;
			}
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
		const Rule* start_rule;
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
						for (State::Cycle& cycle : expanded_leaf->repeated_states)
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
	//					for (State::Cycle& cycle : expanded_leaf->repeated_states)
	//					{
	//						cycle.begin = i;
	//					}
	//				}
	//	if (leafs.size() == 0) throw "Code is invalid";
	//	else if (leafs.size() > 1) throw "Code is ambiguous";
	//	else return leafs.front()->getTree();
	//	//return analyze(input.begin(), input.end()).front()->getTree();
	//}
	std::unique_ptr<TreeNode> analyze(const std::string& code)
	{
		std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule, code) };
		//std::list<std::shared_ptr<State>> end_leafs;
		//while (!leafs.empty())
		for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
		{
			//if (leaf->get()->front_symbol->as<const Terminal*>() == end_symbol) leaf++;
			std::list<std::shared_ptr<State>> expanded_leafs = expand(*leaf);
			for (const std::shared_ptr<State>& expanded_leaf : expanded_leafs)
			{
				const Terminal* front_symbol = expanded_leaf->front_symbol->as<const Terminal*>();
				//std::optional<Token> token = expanded_leaf->tokenizer.getToken(front_symbol->token_type);
				bool has_value = expanded_leaf->tokenizer.checkToken(front_symbol->token_type);
				if (has_value)
				{
					std::shared_ptr<State> advanced_leaf = expanded_leaf->advanced(checkingRules[front_symbol]);
					//advanced_leaf->tokenizer = advanced_leaf->tokenizer.advanced(front_symbol->token_type);
					if (front_symbol == end_symbol)
					{
						//advanced_leaf->getTree()->out(0);
						leafs.push_front(advanced_leaf);
					}
					else
						leafs.push_back(advanced_leaf);

					std::map<const Rule*, State::Cycle> passed_cycles;
					std::map<const Rule*, State::Cycle>::iterator exp_it = std::begin(expanded_leaf->cycles);
					std::map<const Rule*, State::Cycle>::iterator adv_it = std::begin(advanced_leaf->cycles);
					while (exp_it != std::end(expanded_leaf->cycles))
					{
						if (adv_it != std::end(advanced_leaf->cycles) && *exp_it == *adv_it)
						{
							exp_it = std::next(exp_it);
							adv_it = std::next(adv_it);
						}
						else
						{
							passed_cycles.insert(*exp_it);
							exp_it = std::next(exp_it);
						}
					}
					for (const std::pair<const Rule*, State::Cycle>& c : passed_cycles)
					{
						leafs.push_back(c.second.state);
						//leafs.back()->repeated_states = advanced_leaf->repeated_states;
						leafs.back()->cycles.insert_or_assign(c.first, c.second);
					}
				}
			}
		}
		if (leafs.size() == 0) throw "Code is invalid";
		else if (leafs.size() > 1) throw "Code is ambiguous";
		else return leafs.front()->getTree();
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
	Token::Type oper_minus("-", std::regex("-"));
	Token::Type oper_mul("*", std::regex("\\*"));
	Token::Type oper_div("/", std::regex("/"));
	Token::Type oper_eq("=", std::regex("="));
	Token::Type oper_lb("(", std::regex("\\("));
	Token::Type oper_rb(")", std::regex("\\)"));

	const Terminal* s_variable = new Terminal(variable);
	const Terminal* s_integer = new Terminal(integer);
	const Terminal* s_oper_mul = new Terminal(oper_mul);
	const Terminal* s_oper_div = new Terminal(oper_div);
	const Terminal* s_oper_plus = new Terminal(oper_plus);
	const Terminal* s_oper_minus = new Terminal(oper_minus);
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
	Analyzer analyzer({ s_variable,s_integer,s_oper_mul,s_oper_div,s_oper_plus,s_oper_minus,s_oper_eq,s_oper_lb,s_oper_rb,s_expression,s_val,s_sum,s_mul }, s_expression, rules);
	std::string code = "qwe*(pi+42)+34"s;
	std::unique_ptr<TreeNode> root = analyzer.analyze(code);
	root->out(0);

	std::map<std::string, double> variables;
	double pi = 3.14;
	double e = 2.71;
	double qwe = 42;
	variables["pi"s] = pi;
	variables["e"s] = e;
	variables["qwe"s] = qwe;

	std::map<const Rule*, std::function<double(const TreeNode&)>> getDouble;
	std::list<const Rule*>::iterator it_rule = rules.begin();
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[0]->rule])(*node.childs[0]);  return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[0]->rule])(*node.childs[0]);  return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[0]->rule])(*node.childs[0]) + (getDouble[node.childs[2]->rule])(*node.childs[2]);  return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[1]->rule])(*node.childs[1]); return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[0]->rule])(*node.childs[0]);  return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node) { double d = (getDouble[node.childs[0]->rule])(*node.childs[0]) * (getDouble[node.childs[2]->rule])(*node.childs[2]);  return d; };
	getDouble[*(it_rule++)] = [&](const TreeNode& node)
	{
		double d = variables[node.childs[0]->parent->tokenizer.getToken(variable).value().value];  return d;
	};
	getDouble[*(it_rule++)] = [&](const TreeNode& node)
	{
		std::optional<Token> t = node.childs[0]->parent->tokenizer.getToken(integer);
		std::string s = t.value().value;
		double d = std::stod(s);
		//std::cout << d << std::endl;
		return d;
	};
	std::cout << (getDouble[rules.front()])(*root) << "   " << qwe * (pi + 42) + 34;
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

	/*Token::Type a("a", std::regex("a"));
	Token::Type b("b", std::regex("b"));
	const Terminal* s_a = new Terminal(a);
	const Terminal* s_b = new Terminal(b);
	const NonTerminal* s_A = new NonTerminal("A"s);
	const NonTerminal* s_B = new NonTerminal("B"s);
	std::list<const Rule*> rules1
	{	new Rule(s_A, { s_B, s_a }),
		new Rule(s_B, { s_A, s_b }),
		new Rule(s_A, { }),
		new Rule(s_B, { }),
	};
	Analyzer analyzer1({ s_a, s_b, s_A, s_B }, s_A, rules1);
	std::unique_ptr<TreeNode> root1 = analyzer1.analyze("abababa");
	root1->out(0);*/
	return 0;
}