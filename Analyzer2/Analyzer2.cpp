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
#include <tuple>
#include <fstream>
#include <sstream>
#include <typeinfo>
#include "Tokenizer.h";
#include "rtuple.h";
#include "Utils.h";
#include "AnalyzerBase.h"

using namespace std::string_literals;

using Entity = std::variant<int, double, bool, std::string>;

template<class Start>
class Analyzer: public AnalyzerBase
{
public:
	Analyzer(std::list<Symbol> symbols, const NonTerminal* start, std::list<const Rule0*> rules):
		AnalyzerBase(symbols,
			//new Rule2<nullptr_t, Start, Token>(nullptr, { start, end_symbol, nullptr }, start->dbg_name),
			rules)
	{
		start_rule = new Rule2<nullptr_t, Start, Token>(nullptr, { start, end_symbol, nullptr }, start->dbg_name);

		RuleNode* rn = &rootRuleNodes[start_rule->left];
		rn->symbol = start;
		startRuleNode = rn;
		rn = &rn->continuations[end_symbol];
		rn->symbol = end_symbol;
		rn->rule = start_rule;

		//for (const Symbol& symbol : symbols)
		//{
		//	if (symbol.is<const Terminal*>())
		//		checkingRules[symbol.as<const Terminal*>()] = Rule2<Token>::makeChecking(symbol.as<const Terminal*>());
		//}
		//checkingRules[end_symbol] = Rule2<Token>::makeChecking(end_symbol);
	}
	TreeNode1<Start>* analyze(const std::string& code)
	{
		std::unique_ptr<HistoryState> hist = AnalyzerBase::analyze(code);
		TreeNode2<nullptr_t, Start, Token>* start_node = new TreeNode2<nullptr_t, Start, Token>(hist.get(), std::make_index_sequence<2>());
		return std::get<0>(start_node->childs);
	}
};
template<class Start>
class AnalyzerBuilder
{
	std::map<std::string, Token::Type> token_types;
	std::map<std::string, const Terminal*> terminals;
	std::map<std::string, const NonTerminal*> non_terminals;
	std::map<std::string, Symbol> symbols;
	//std::list<Symbol> symbols;
	std::map<std::string, const std::type_info*> symbol_types;
	std::list<const Rule0*> rules;
	struct RuleStrings
	{
		std::string left;
		std::vector<std::string> right;
		RuleStrings(std::string left, std::vector<std::string> right):
			left(left),
			right(right)
		{}
	};
	Token::Type token_terminal;
	Token::Type token_non_terminal;
	Token::Type token_arrow;
	const Terminal* terminal_terminal;
	const Terminal* terminal_non_terminal;
	const Terminal* terminal_arrow;
	const NonTerminal* non_terminal_rule;
	const NonTerminal* non_terminal_symbol;
	const NonTerminal* non_terminal_symbol_sequence;

	std::list<const Rule0*> analyzer_rules;
	Analyzer<RuleStrings> rules_analyzer;
	const NonTerminal* getNonTerminal(std::string name)
	{
		if (non_terminals.find(name) == non_terminals.end())
		{
			non_terminals.insert(std::pair(name, new NonTerminal(name)));
			symbols.insert(std::pair(name, non_terminals[name]));
			//symbols.push_back(non_terminals[name]);
		}
		return non_terminals.at(name);
	}
	Token::Type getTokenType(std::string name)
	{
		if (token_types.find(name) == token_types.end())
		{
			token_types.emplace(name, Token::Type(name, std::regex(regex_escape(name))));
			terminals.insert(std::pair(name, new Terminal(token_types.at(name))));
			symbols.insert(std::pair(name, terminals[name]));
			//symbols.push_back(terminals[name]);
		}
		return token_types.at(name);
	}
	const Terminal* getTerminal(std::string name)
	{
		getTokenType(name);
		return terminals.at(name);
	}
	template<class T>
	void checkType(std::string symbol)
	{
		using child_type = ChildFiller<T>::rule_type;
		if (symbol_types.find(symbol) == symbol_types.end())
		{
			symbol_types.insert(std::pair(symbol, &typeid(child_type)));
		}
		else if (symbol_types.at(symbol) != &typeid(child_type))
		{
			throw "Types mismatch: "s + symbol + " and "s + typeid(T).name();
		}
	}
	template<class T, class... Ts>
	void checkTypes(std::vector<std::string>::const_iterator i)
	{
		using child_type = ChildFiller<T>::rule_type;
		checkType<child_type>(*i);
		if constexpr (sizeof...(Ts) > 0)
			checkTypes<Ts...>(std::next(i));
	}
public:
	AnalyzerBuilder():
		token_terminal("terminal", std::regex("[[:lower:][:punct:]]+")),
		token_non_terminal("non_terminal", std::regex("[[:upper:]][[:alnum:]]*")),
		token_arrow("arrow", std::regex("->")),
		//token_spaces("token_spaces", std::regex("\\s+")),
		terminal_terminal(new Terminal(token_terminal)),
		terminal_non_terminal(new Terminal(token_non_terminal)),
		terminal_arrow(new Terminal(token_arrow)),
		//terminal_spaces(new Terminal(token_spaces)),
		non_terminal_rule(new NonTerminal("Rule"s)),
		non_terminal_symbol(new NonTerminal("Symbol"s)),
		non_terminal_symbol_sequence(new NonTerminal("Symbol_sequence"s)),
		analyzer_rules
		{
			new Rule2<std::string, Token>(non_terminal_symbol, { terminal_terminal },
			[this](Token t)
				{
					getTerminal(t.value);
					return t.value;
				}),
			new Rule2<std::string, Token>(non_terminal_symbol, { terminal_non_terminal },
			[this](Token t)
				{
					getNonTerminal(t.value);
					return t.value;
				}),
			new Rule2<std::list<std::string>, std::string, std::list<std::string>>(non_terminal_symbol_sequence, { non_terminal_symbol, /*terminal_spaces,*/ non_terminal_symbol_sequence },
				[this](std::string symbol, std::list<std::string> list_symbol)
				{
					list_symbol.push_front(symbol);
					return list_symbol;
				}),
			new Rule2<std::list<std::string>>(non_terminal_symbol_sequence, { },
				[this]()
				{
					return std::list<std::string>{};
				}),
			new Rule2<RuleStrings, Token, Token, std::list<std::string>>(non_terminal_rule, { terminal_non_terminal, terminal_arrow, non_terminal_symbol_sequence },
				[this](Token nt, Token, std::list<std::string> list_symbol)
				{
					return RuleStrings(nt.value, std::vector<std::string>(list_symbol.begin(), list_symbol.end()));
				})
		},
		//analyzer_rules
		//{
		//	new Rule2<Symbol, Token>(non_terminal_symbol, { terminal_terminal },
		//	[this](Token t)
		//		{
		//			return getTerminal(t.value);
		//		}),
		//	new Rule2<Symbol, Token>(non_terminal_symbol, { terminal_non_terminal },
		//	[this](Token t)
		//		{
		//			return getNonTerminal(t.value);
		//		}),
		//	new Rule2<std::list<Symbol>, Symbol, std::list<Symbol>>(non_terminal_symbol_sequence, { non_terminal_symbol, /*terminal_spaces,*/ non_terminal_symbol_sequence },
		//		[this](Symbol symbol, std::list<Symbol> list_symbol)
		//		{
		//			list_symbol.push_front(symbol);
		//			return list_symbol;
		//		}),
		//	new Rule2<std::list<Symbol>>(non_terminal_symbol_sequence, { },
		//		[this]()
		//		{
		//			return std::list<Symbol>{};
		//		}),
		//	new Rule2<RuleBuilder, Token, Token, std::list<Symbol>>(non_terminal_rule, { terminal_non_terminal, terminal_arrow, non_terminal_symbol_sequence },
		//		[this](Token nt, Token, std::list<Symbol> list_symbol)
		//		{
		//			return RuleBuilder(getNonTerminal(nt.value), std::vector<Symbol>(list_symbol.begin(), list_symbol.end()));
		//		})
		//},
		rules_analyzer(
			{ terminal_terminal , terminal_non_terminal ,terminal_arrow , non_terminal_symbol,non_terminal_rule,non_terminal_symbol,non_terminal_symbol_sequence },
			non_terminal_rule, analyzer_rules)
	{}
	AnalyzerBuilder& add_token_type(std::string name, std::string regex)
	{
		token_types.emplace(name, Token::Type(name, std::regex(regex)));
		terminals[name] = new Terminal(token_types.at(name));
		symbols.insert(std::pair(name, terminals[name]));
		return *this;
	}
	//AnalyzerBuilder& add_non_terminal(std::string name)
	//{
	//	non_terminals[name] = new NonTerminal(name);
	//	symbols.push_back(non_terminals[name]);
	//	return *this;
	//}
	//template<class R, class... Args>
	//AnalyzerBuilder& add_rule(std::string s_rule, const R(*get)(Args...))
	//{
	//	RuleBuilder rule_builder(rules_analyzer.analyze(s_rule)->get());
	//	//if (rule_builder.right.size() != sizeof...(Childs)) throw "Rule representation is incompatible with get signature";
	//	const Rule0* rule = new Rule2/*<T, Childs...>*/(rule_builder.left, rule_builder.right, get);
	//	rules.push_back(rule);
	//	return *this;
	//}
	template<class T, class... Childs>
	AnalyzerBuilder& add_rule(std::string s_rule, std::function<T(Childs...)> get)
	{
		TreeNode1<RuleStrings>* tn = rules_analyzer.analyze(s_rule);
		RuleStrings rule_strings(tn->get());
		const NonTerminal* left = getNonTerminal(rule_strings.left);
		std::vector<Symbol> right;
		for (const std::string& r : rule_strings.right)
			right.push_back(symbols.at(r));
		if (rule_strings.right.size() != sizeof...(Childs)) throw "Types mismatch: "s + std::to_string(rule_strings.right.size()) + " and "s + std::to_string(sizeof...(Childs));
		checkType<T>(rule_strings.left);
		if constexpr (sizeof...(Childs) > 0)
			checkTypes<Childs...>(rule_strings.right.cbegin());
		const Rule0* rule = new Rule2<T, Childs...>(left, right, get);
		rules.push_back(rule);
		return *this;
	}
	Analyzer<Start> build(std::string start)
	{
		std::list<Symbol> symbols_list;
		for (const std::pair<std::string, Symbol>& s : symbols)
			symbols_list.push_back(s.second);
		return Analyzer<Start>(symbols_list, getNonTerminal(start), rules);
	}
};

struct Scope;
struct Function
{
	std::vector<std::string> arguments;
	//virtual void get(std::shared_ptr<Scope> scope) = 0;
	virtual void get(std::shared_ptr<Scope> scope, std::vector<Entity> args) = 0;
};
struct Scope
{
	std::map<std::string, Entity> variables;
	std::map<std::string, Function*> functions;
	std::shared_ptr<Scope> parent;
	Scope():
		parent(nullptr)
	{}
	Scope(std::shared_ptr<Scope> parent):
		parent(parent)
	{}
	Entity& variable(std::string name)
	{
		Scope* scope = this;
		while (scope != nullptr)
		{
			if (scope->variables.find(name) != scope->variables.end()) break;
			scope = scope->parent.get();
		}
		if (scope == nullptr) scope = this;
		return scope->variables[name];

		//if (variables.find(name) != variables.end())
		//	return variables.at(name);
		//else return parent->variable(name);
	}
	Function*& function(std::string name)
	{
		Scope* scope = this;
		while (scope != nullptr)
		{
			if (scope->functions.find(name) != scope->functions.end()) break;
			scope = scope->parent.get();
		}
		if (scope == nullptr) scope = this;
		return scope->functions[name];
	}
};
struct LangFunction: Function
{
	TreeNode1<nullptr_t>* body;
	//void get(std::shared_ptr<Scope> scope) override
	void get(std::shared_ptr<Scope> scope, std::vector<Entity> args) override
	{
		body->get();
	}
};
template<class... Args>
struct BuiltinFunction: Function
{
	std::function<void(Args...)> function;
	template <std::size_t... Is>
	//std::tuple<Args...> fill_args(std::shared_ptr<Scope> scope, std::index_sequence<Is...>)
	std::tuple<Args...> fill_args(std::vector<Entity> args, std::index_sequence<Is...>)
	{
		//return std::tuple<Args...>(std::get<Args>(scope->variable("arg_"s + std::to_string(Is)))...);
		return std::tuple<Args...>(fill_arg<Args>(args[Is])...);
	}
	template<class T>
	T fill_arg(Entity e)
	{
		return std::get<T>(e);
	}
	template<>
	Entity fill_arg(Entity e)
	{
		return e;
	}
	//void get(std::shared_ptr<Scope> scope) override
	void get(std::shared_ptr<Scope> scope, std::vector<Entity> args) override
	{
		std::tuple<Args...> tuple_args = fill_args(args, std::index_sequence_for<Args...>());
		return std::apply(function, tuple_args);
	}
};


int main()
{
	//std::map<std::string, Entity> variables;
	double pi = 3.14;
	double e = 2.71;
	double qwe = 42;

	//std::string t = typeid(TreeNode2<int, double>).name();

	std::shared_ptr<Scope> global_scope = std::make_shared<Scope>();
	global_scope->variables["pi"s] = pi;
	global_scope->variables["e"s] = e;
	global_scope->variables["qwe"s] = qwe;
	BuiltinFunction<Entity> print_function;
	print_function.function = [](Entity e) { std::visit([](auto& s) { std::cout << s; }, e); };
	//BuiltinFunction<std::string> print_function;
	//print_function.function = [](std::string s) { std::cout << s; };
	print_function.arguments = { "s" };
	global_scope->function("print"s) = &print_function;
	std::shared_ptr<Scope> current_scope = global_scope;

	auto forward = [](auto v) { return v; };

	AnalyzerBuilder<nullptr_t> ab;
	ab
		.add_token_type("variable", "[[:alpha:]][[:alnum:]]*")
		.add_token_type("integer", "[[:digit:]]+")
		.add_token_type("string", "'[\\w\\W]+?'")
		.add_token_type("out", "out\\b")
		.add_token_type("comment", "#.*")
		;
	ab.add_rule("Program->Statement",
		std::function([](TreeNode1<nullptr_t>* st) -> nullptr_t
			{
				st->get();
				return nullptr;
			}));

	ab.add_rule("Statement->comment",
		std::function([](Token) -> nullptr_t
			{
				return nullptr;
			}));
	ab.add_rule("Statement->Expression ;"s,
		std::function([](TreeNode1<nullptr_t>* e, Token)-> nullptr_t
			{
				e->get();
				return nullptr;
			}));
	ab.add_rule("Statement->{ StatementList }",
		std::function([&](Token, std::list<TreeNode1<nullptr_t>*> statement_list, Token) -> nullptr_t
			{
				current_scope = std::make_shared<Scope>(current_scope);
				for (TreeNode1<nullptr_t>* s : statement_list)
					s->get();
				current_scope = current_scope->parent;
				return nullptr;
			}));
	ab.add_rule("Statement->if BoolExpression Statement"s,
		std::function([](Token, bool cond, TreeNode1<nullptr_t>* stat) -> nullptr_t
			{
				if (cond)
					stat->get();
				return nullptr;
			}));
	ab.add_rule("Statement->if BoolExpression Statement else Statement"s,
		std::function([](Token, bool cond, TreeNode1<nullptr_t>* stat_true, Token, TreeNode1<nullptr_t>* stat_false) -> nullptr_t
			{
				if (cond)
					stat_true->get();
				else
					stat_false->get();
				return nullptr;
			}));

	ab.add_rule("StatementList->",
		std::function([]()
			{
				return std::list<TreeNode1<nullptr_t>*>{};
			}));
	//ab.add_rule("StatementList->Statement",
	//	std::function([](TreeNode1<nullptr_t>* statement)->std::list<TreeNode1<nullptr_t>*>
	//		{
	//			return std::list<TreeNode1<nullptr_t>*>{statement};
	//		}));
	ab.add_rule("StatementList->StatementList Statement",
		std::function([](std::list<TreeNode1<nullptr_t>*> statement_list, TreeNode1<nullptr_t>* statement)->std::list<TreeNode1<nullptr_t>*>
			{
				statement_list.push_back(statement);
				return statement_list;
			}));
	ab.add_rule("Statement->for ( Expression ; BoolExpression ; Expression ) Statement"s,
		std::function([](Token, Token, TreeNode1<nullptr_t>* init, Token, TreeNode1<bool>* cond, Token, TreeNode1<nullptr_t>* iter, Token, TreeNode1<nullptr_t>* stat) -> nullptr_t
			{
				for (init->get(); cond->get(); iter->get())
					stat->get();
				return nullptr;
			}));
	ab.add_rule("Statement->while BoolExpression Statement"s,
		std::function([](Token, TreeNode1<bool>* cond, TreeNode1<nullptr_t>* stat) -> nullptr_t
			{
				while (cond->get())
					stat->get();
				return nullptr;
			}));

	ab.add_rule("Statement->def variable ( ParametersList ) Statement"s,
		std::function([&](Token, Token name, Token, std::list<std::string> params, Token, TreeNode1<nullptr_t>* body) -> nullptr_t
			{
				LangFunction* func = new LangFunction();
				func->arguments.assign(params.begin(), params.end());
				func->body = body;
				current_scope->function(name.value) = func;
				return nullptr;
			}));

	ab.add_rule("ParametersList->",
		std::function([]()->std::list<std::string>
			{
				return std::list<std::string>{};
			}));
	ab.add_rule("ParametersList->variable",
		std::function([](Token par)->std::list<std::string>
			{
				return std::list<std::string>{par.value};
			}));
	ab.add_rule("ParametersList->ParametersList , variable",
		std::function([](std::list<std::string> parameters_list, Token, Token par)->std::list<std::string>
			{
				parameters_list.push_back(par.value);
				return parameters_list;
			}));

	ab.add_rule("Expression->out Value"s,
		std::function([](Token, Entity e)-> nullptr_t
			{
				std::visit([](auto& v) { std::cout << v; }, e);
				//std::cout << d /*<< std::endl*/;
				return nullptr;
			}));
	ab.add_rule("Expression->outln"s,
		std::function([](Token)-> nullptr_t
			{
				std::cout << std::endl;
				return nullptr;
			}));
	ab.add_rule("Expression->get variable"s,
		std::function([&](Token, Token v)-> nullptr_t
			{
				double d;
				std::cin >> d;
				current_scope->variable(v.value) = d;
				return nullptr;
			}));
	ab.add_rule("Expression->variable = Value"s,
		std::function([&](Token v, Token, Entity e)-> nullptr_t
			{
				current_scope->variable(v.value) = e;
				return nullptr;
			}));
	ab.add_rule("Expression->variable ( ValuesList )"s,
		std::function([&](Token func, Token, std::list<Entity> args, Token)-> nullptr_t
			{
				Function* function = current_scope->function(func.value);
				std::shared_ptr<Scope> return_scope = current_scope;
				std::shared_ptr<Scope> function_scope = std::make_shared<Scope>(global_scope);
				std::vector<Entity> v_args(args.begin(), args.end());
				for (int i = 0; i < args.size(); i++)
				{
					function_scope->variables[function->arguments[i]] = v_args[i];
					function_scope->variables["arg_"s + std::to_string(i)] = v_args[i];
				}
				current_scope = function_scope;
				function->get(function_scope, v_args);
				current_scope = return_scope;
				return nullptr;
			}));

	ab.add_rule("ValuesList->",
		std::function([]()->std::list<Entity>
			{
				return std::list<Entity>{};
			}));
	ab.add_rule("ValuesList->Value",
		std::function([](Entity val)->std::list<Entity>
			{
				return std::list<Entity>{val};
			}));
	ab.add_rule("ValuesList->ValuesList , Value",
		std::function([](std::list<Entity> values_list, Token, Entity val)->std::list<Entity>
			{
				values_list.push_back(val);
				return values_list;
			}));

	//ab.add_rule("Value->integer"s,
	//	std::function([](Token i)->Entity
	//		{
	//			return std::stod(i.value);
	//		}));
	//ab.add_rule("Value->variable"s,
	//	std::function([&](Token v)->Entity
	//		{
	//			return variables[v.value];
	//		}));
	ab.add_rule("Value->string"s,
		std::function([&](Token s)->Entity
			{
				return s.value.substr(1, s.value.size() - 2);
			}));
	ab.add_rule("Value->rand Value"s,
		std::function([&](Token, Entity l)->Entity
			{
				return double(std::rand() % int(std::get<double>(l)));
			}));
	ab.add_rule("Value->ArithmeticExpression"s, std::function([](Entity e)->Entity { return e; }));

	ab.add_rule("BoolExpression->ArithmeticExpression == ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 == v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression != ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 != v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression < ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 < v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression > ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 > v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression <= ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 <= v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression >= ArithmeticExpression"s,
		std::function([](Entity v1, Token, Entity v2) -> bool
			{
				if (v1.index() != v2.index()) throw "Types mismath";
				return v1 >= v2;
			}));
	ab.add_rule("BoolExpression->ArithmeticExpression"s,
		std::function([](Entity e) -> bool
			{
				return std::get<bool>(e);
			}));
	ab.add_rule("BoolExpression->true"s,
		std::function([](Token) -> bool
			{
				return true;
			}));
	ab.add_rule("BoolExpression->false"s,
		std::function([](Token) -> bool
			{
				return false;
			}));

	ab.add_rule("ArithmeticExpression->Summand", std::function([](Entity e)->Entity { return e; }));

	std::string sum_rule = "ArithmeticExpression->ArithmeticExpression + Summand"s;
	//std::string sum_rule = "ArithmeticExpression->Summand + ArithmeticExpression"s;
	ab.add_rule(sum_rule, std::function([](Entity e1, Token, Entity e2)->Entity
		{
			return std::visit([](auto& d1, auto& d2)->Entity
				{
					if constexpr (std::is_same_v<decltype(d1), decltype(d2)>)
						return d1 + d2;
					else throw "Types mismath";
				}, e1, e2);
		}));

	ab.add_rule("ArithmeticExpression->ArithmeticExpression - Summand", std::function([](Entity e1, Token, Entity e2)->Entity
		{
			return std::visit([](auto& d1, auto& d2)->Entity
				{
					if constexpr (std::is_same_v<decltype(d1), decltype(d2)> && !std::is_same_v<decltype(d1), std::string&>)
						return d1 - d2;
					else throw "Types mismath";
				}, e1, e2);
		}));

	ab.add_rule("Summand->Multiplier", std::function([](Entity e)->Entity { return e; }));
	ab.add_rule("Summand->Summand * Multiplier", std::function([](Entity e1, Token, Entity e2)->Entity
		{
			return std::visit([](auto& d1, auto& d2)->Entity
				{
					if constexpr (std::is_same_v<decltype(d1), decltype(d2)> && !std::is_same_v<decltype(d1), std::string&>)
						return d1 * d2;
					else throw "Types mismath";
				}, e1, e2);
		}));
	ab.add_rule("Summand->Summand / Multiplier", std::function([](Entity e1, Token, Entity e2)->Entity
		{
			return std::visit([](auto& d1, auto& d2)->Entity
				{
					if constexpr (std::is_same_v<decltype(d1), decltype(d2)> && !std::is_same_v<decltype(d1), std::string&>)
						return d1 / d2;
					else throw "Types mismath";
				}, e1, e2);
		}));
	ab.add_rule("Summand->Summand // Multiplier", std::function([](Entity e1, Token, Entity e2)->Entity
		{
			return std::visit([](auto& d1, auto& d2)->Entity
				{
					if constexpr (std::is_same_v<decltype(d1), decltype(d2)> && !std::is_same_v<decltype(d1), std::string&>)
						return double(int(d1) / int(d2));
					else throw "Types mismath";
				}, e1, e2);
		}));

	ab.add_rule("Multiplier->( ArithmeticExpression )", std::function([](Token, Entity d, Token)->Entity { return d; }));
	ab.add_rule("Multiplier->integer", std::function([](Token i)->Entity { return std::stod(i.value); }));
	ab.add_rule("Multiplier->variable", std::function([&](Token v)->Entity { return current_scope->variable(v.value); }));
	;
	Analyzer<nullptr_t> an = ab.build("Program"s);
	std::ifstream code_file("Code.txt");
	std::stringstream code_stream;
	code_stream << code_file.rdbuf();
	std::string code = code_stream.str();

	std::srand(std::time(nullptr));
	//variables["v"s] = std::rand() % 100;

	//TreeNode1<nullptr_t>* program = an.analyze("{print ((1+2));}");
	//program->out(0);
	//program->get();

	AnalyzerBuilder<nullptr_t> abtest;
	//abtest.add_rule("A->a A B", std::function([](Token, int i, int i1)->int { return i + i1; }));
	//abtest.add_rule("B->b", std::function([](Token)->int { return 1; }));
	//abtest.add_rule("B->c", std::function([](Token)->int { return 1; }));
	//abtest.add_rule("B->d", std::function([](Token)->int { return 1; }));
	//abtest.add_rule("B->e", std::function([](Token)->int { return 1; }));
	//abtest.add_rule("A->", std::function([]()->int { return 0; }));
	abtest.add_rule("A->a A b", std::function([](Token, int i, Token)->int { return i + 1; }));
	abtest.add_rule("A->a A c", std::function([](Token, int i, Token)->int { return i + 1; }));
	abtest.add_rule("A->a A d", std::function([](Token, int i, Token)->int { return i + 1; }));
	abtest.add_rule("A->a A e", std::function([](Token, int i, Token)->int { return i + 1; }));
	abtest.add_rule("A->", std::function([]()->int { return 0; }));
	TreeNode1<nullptr_t>* programtest = abtest.build("A"s).analyze("a a a a a a a a b b b b b d c e");
	programtest->out(0);

	return 0;
}
