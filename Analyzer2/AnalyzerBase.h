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

using namespace std::string_literals;

struct Terminal;
struct NonTerminal;
struct Symbol;

struct State;
struct HistoryState;

struct Rule0;
struct RuleNode;
template <class T> struct Rule1;
template <class T, class... Childs> struct Rule2;

template<class T>
struct ChildFiller;

struct TreeNode0;
template<class T> struct TreeNode1;
template<class T, class... Childs> struct TreeNode2;



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
struct Symbol: std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>
{
	std::string dbg_name;
	Symbol();
	Symbol(nullptr_t);
	Symbol(const Terminal* terminal);
	Symbol(const NonTerminal* non_terminal);
	bool isTerminal() const;
	Symbol* operator->();
	template <class T>
	bool is() const;
	template <class T>
	T as() const;
};

struct State: std::enable_shared_from_this<State>
{
	//struct SymbolPointer
	//{
	//	const State* state;
	//	std::vector<Symbol>::const_iterator iterator;
	//	const RuleNode* ruleNode;
	//	SymbolPointer(const State* state, std::vector<Symbol>::const_iterator iterator);
	//	SymbolPointer(const RuleNode* ruleNode, std::vector<Symbol>::const_iterator iterator);
	//	SymbolPointer(std::nullptr_t = nullptr);
	//	Symbol getSymbol() const;
	//	Symbol operator->() const;
	//	Symbol operator*() const;
	//};

	using SymbolPointer = const State*;

	const RuleNode* ruleNode;
	const std::shared_ptr<const State> prev;
	SymbolPointer parent;
	const Tokenizer tokenizer;
	std::list<const State*> dbg_hist;
	std::list<std::string> dbg_stack;
	int dbg_depth;
	//struct Cycle
	//{
	//	std::shared_ptr<State> state;
	//	std::shared_ptr<State> begin;
	//	//bool passed = false;
	//	friend bool operator == (const Cycle& c1, const Cycle& c2);
	//	Cycle(std::shared_ptr<State> state, std::shared_ptr<State> begin);
	//	Cycle(std::shared_ptr<State> state);
	//};
	//std::map<const Rule0*, Cycle> cycles;

	State(const Rule0* rule, std::shared_ptr<const State> prev, SymbolPointer parent);
	State(const RuleNode* ruleNode, std::shared_ptr<const State> prev, SymbolPointer parent);
	static std::shared_ptr<const State> make(const RuleNode* ruleNode, std::shared_ptr<const State> prev, SymbolPointer parent);
	State(const RuleNode* startRuleNode, const std::string& code);
	bool isParentOf(const State& mbChild) const;
	bool isChildOf(const State& mbParent) const;
	std::list<std::shared_ptr<const State>> expanded(const RuleNode* rootRuleNode) const;
	std::list<std::shared_ptr<const State>> continued(std::shared_ptr<const State> current) const;
	std::unique_ptr<HistoryState> getTree() const;
};
struct HistoryState
{
	//std::shared_ptr<const State> state;
	const Rule0* rule;
	const Tokenizer tokenizer;
	HistoryState* parent;
	std::vector<std::unique_ptr<HistoryState>> childs;
	HistoryState(std::shared_ptr<const State> state);
	void out(int offset) const;
};


struct Rule0
{
	Symbol left;
	std::vector<Symbol> right;
	std::string dbg_view;
	Rule0(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view);
	Rule0(const NonTerminal* left, const std::vector<Symbol>& right);
	//static Rule0* makeStart(const NonTerminal* start, const Terminal* end);
	//virtual TreeNode0* make_tree_node(const HistoryState* state) const = 0;
};

struct RuleNode
{
	//const RuleNode* parent;
	const Rule0* rule;
	Symbol symbol;
	std::map<Symbol, RuleNode> continuations;

	RuleNode();
	RuleNode(const RuleNode&) = delete;
};

template <class T>
struct Rule1: Rule0
{
	Rule1(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view);
	Rule1(const NonTerminal* left, const std::vector<Symbol>& right);
	virtual TreeNode1<T>* make_tree_node(const HistoryState* state) const = 0;
};

template <class T, class... Childs>
struct Rule2: Rule1<T>
{
	Rule2(Symbol left, const std::vector<Symbol>& right, const std::string& view);
	Rule2(const NonTerminal* left, const std::vector<Symbol>& right, std::function<T(Childs...)> get);
	std::function<T(Childs...)> get;
	TreeNode1<T>* make_tree_node(const HistoryState* state) const override;
};

template<>
struct Rule2<Token>: Rule1<Token>
{
	Rule2(const Terminal* left);
	TreeNode1<Token>* make_tree_node(const HistoryState* state) const override;
	static Rule0* makeChecking(const Terminal* terminal);
};


struct TreeNode0
{
	TreeNode0* parent;
	const Rule0* rule;
	virtual void out(int offset) const = 0;
	TreeNode0(const HistoryState* state);
};

template<class T>
struct TreeNode1: public TreeNode0
{
	TreeNode1(const HistoryState* state);
	virtual void out(int offset) const override = 0;
	virtual T get() = 0;
};

template<class T>
struct ChildFiller
{
	using stored_type = TreeNode1<T>*;
	using rule_type = T;
	static T get(stored_type node);
};
template<class T>
struct ChildFiller<TreeNode1<T>*>
{
	using stored_type = TreeNode1<T>*;
	using rule_type = T;
	static TreeNode1<T>* get(stored_type node);
};

template<class T, class... Childs>
struct TreeNode2: public TreeNode1<T>
{
	
	std::tuple<typename ChildFiller<Childs>::stored_type...> childs;
	template <size_t... inds>
	TreeNode2(const HistoryState* state, std::index_sequence<inds...>);
	template <std::size_t... Is>
	void out_childs(int offset, std::index_sequence<Is...>) const;
	void out(int offset) const override;
	template <std::size_t... Is>
	std::tuple<Childs...> fill_childs(std::index_sequence<Is...>);
	T get() override;
};

template<>
struct TreeNode2<Token>: TreeNode1<Token>
{
	TreeNode2(const HistoryState* state);
	Token token;
	Token get() override;
	void out(int offset) const override;
};

template<class T>
struct TreeNode2<TreeNode1<T>*>:TreeNode1<TreeNode1<T>*>
{
	TreeNode2(const HistoryState* state);
	const TreeNode1<T>* node;
	const TreeNode1<T>* get() override;
	void out(int offset) const override;
};



class AnalyzerBase
{
protected:
	//std::map<std::string, Symbol> symbolbyName;
	const Terminal* empty_symbol;
	const Terminal* end_symbol;
	const Rule0* start_rule;
	//std::map<Symbol, RuleNode> rootRuleNodes;
	RuleNode rootRuleNode;
	RuleNode* startRuleNode;
	std::map<const NonTerminal*, std::list<const Rule0*>> expandingRules;
	std::map<const Terminal*, Rule0*> checkingRules;
	//std::list<std::shared_ptr<State>> expand(const std::shared_ptr<State>& leaf);
public:
	AnalyzerBase(std::list<Symbol> symbols, /*const Rule0* start_rule, std::map<const Terminal*, Rule0*> checkingRules, */ std::list<const Rule0*> rules);
	std::unique_ptr<HistoryState> analyze(const std::string& code);
};

#include "AnalyzerBase.hpp"