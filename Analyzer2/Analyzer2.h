#pragma once
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
#include "Tokenizer.h";
#include "rtuple.h";
#include "Utils.h";
//export module Analyzer;

using namespace std::string_literals;

struct Terminal;
struct NonTerminal;
struct Symbol;
struct State;
struct HistoryState;
struct RuleBaseBase;
template <class T, class... Childs>
struct Rule;
struct TreeNodeBaseBase;
template<class T>
struct TreeNodeBase;
template<class T, class... Childs>
struct TreeNode;

struct Terminal
{
	Token::Type token_type;
	Terminal(Token::Type token_type);
};
struct NonTerminal
{
	std::string dbg_name;
	NonTerminal(std::string dbg_name);
};
struct Symbol: std::variant<nullptr_t, const Terminal*, const NonTerminal*>
{
	std::string dbg_name;
	Symbol(nullptr_t);
	Symbol(const Terminal* terminal);
	Symbol(const NonTerminal* non_terminal);
	bool isTerminal() const;
	Symbol* operator->();
	template <class T> bool is() const;
	template <class T> T as() const;
};

struct RuleBaseBase
{
	Symbol left;
	std::vector<Symbol> right;
	std::string dbg_view;
	RuleBaseBase(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view);
	RuleBaseBase(const NonTerminal* left, const std::vector<Symbol>& right);
	//static RuleBaseBase* makeStart(const NonTerminal* start, const Terminal* end);
	static RuleBaseBase* makeChecking(const Terminal* terminal);
	//virtual TreeNodeBaseBase* make_tree_node(const HistoryState* state) const = 0;
};
template <class T>
struct RuleBase: RuleBaseBase
{
	RuleBase(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view);
	RuleBase(const NonTerminal* left, const std::vector<Symbol>& right);
	virtual TreeNodeBase<T>* make_tree_node(const HistoryState* state) const = 0;
};
template <class T, class... Childs>
struct Rule: RuleBase<T>
{
	Rule(Symbol left, const std::vector<Symbol>& right, const std::string& view);
	Rule(const NonTerminal* left, const std::vector<Symbol>& right, std::function<T(Childs...)> get);
	std::function<T(const TreeNodeBase<T>&)> getb;
	std::function<T(Childs...)> get;
	TreeNodeBase<T>* make_tree_node(const HistoryState* state) const override;
};
template<>
struct Rule<Token>: RuleBase<Token>
{
	Rule(const Terminal* left);
	TreeNodeBase<Token>* make_tree_node(const HistoryState* state) const override;
};
template <class T, class... Childs>
Rule(const NonTerminal*, const std::vector<Symbol>&, std::function<T(Childs...)>) -> Rule<T, Childs...>;


struct State: std::enable_shared_from_this<State>
{
	struct SymbolPointer
	{
		const State* state;
		std::vector<Symbol>::const_iterator iterator;
		SymbolPointer(const State* state, std::vector<Symbol>::const_iterator iterator);
		SymbolPointer(std::nullptr_t = nullptr);
		Symbol getSymbol() const;
		Symbol operator->() const;
		Symbol operator*() const;
	};

	const RuleBaseBase* rule;
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
		friend bool operator == (const Cycle& c1, const Cycle& c2);
		Cycle(std::shared_ptr<State> state, std::shared_ptr<State> begin);
		Cycle(std::shared_ptr<State> state);
	};
	std::map<const RuleBaseBase*, Cycle> cycles;

	State(const RuleBaseBase* rule, std::shared_ptr<const State> prev, SymbolPointer parent);
	State(const RuleBaseBase* start_rule, const std::string& code);
	bool isParentOf(const State& mbChild) const;
	bool isChildOf(const State& mbParent) const;
	bool ruleApplicable(const RuleBaseBase* rule) const;
	std::shared_ptr<State> advanced(const RuleBaseBase* rule) const;
	std::unique_ptr<HistoryState> getTree() const;
};
struct HistoryState
{
	const State* state;
	HistoryState* parent;
	std::vector<std::unique_ptr<HistoryState>> childs;
};


struct TreeNodeBaseBase
{
	TreeNodeBaseBase* parent;
	const RuleBaseBase* rule;
	//Tokenizer tokenizer;
	TreeNodeBaseBase(const HistoryState* state);
};
template<class T>
struct TreeNodeBase: public TreeNodeBaseBase
{
	//std::vector<std::unique_ptr<TreeNodeBase>> childs;
	//TreeNodeBase();
	TreeNodeBase(const HistoryState* state);
	void out(int offset) const;
	virtual T get() = 0;
	//virtual Tokenizer fill(Tokenizer t) = 0;
};
template<class T, class... Childs>
struct TreeNode: public TreeNodeBase<T>
{
	std::tuple<TreeNodeBase<Childs>*...> childs;
	template <size_t... inds>
	TreeNode(const HistoryState* state, std::index_sequence<inds...>);
	template <std::size_t... Is>
	std::tuple<Childs...> fill_childs(std::tuple<Childs...>& t, std::index_sequence<Is...>);
	std::tuple<Childs...> fill_childs(std::tuple<Childs...>& t);
	T get() override;
	//Tokenizer fill(Tokenizer t) override;
};
template<>
struct TreeNode<Token>: TreeNodeBase<Token>
{
	TreeNode(const HistoryState* state);
	Token token;
	Token get() override;
	//Tokenizer fill(Tokenizer t) override;
};