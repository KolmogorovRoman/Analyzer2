#include "Analyzer2.h"

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
Terminal::Terminal(Token::Type token_type):
	token_type(token_type)
{}

std::string dbg_name;
NonTerminal::NonTerminal(std::string dbg_name):
	dbg_name(dbg_name)
{}

Symbol::Symbol(nullptr_t):
	std::variant<nullptr_t, const Terminal*, const NonTerminal*>(nullptr),
	dbg_name(""s)
{}
Symbol::Symbol(const Terminal* terminal):
	std::variant<nullptr_t, const Terminal*, const NonTerminal*>(terminal),
	dbg_name(terminal->token_type.dbg_name)
{}
Symbol::Symbol(const NonTerminal* non_terminal):
	std::variant<nullptr_t, const Terminal*, const NonTerminal*>(non_terminal),
	dbg_name(non_terminal->dbg_name)
{}
bool Symbol::isTerminal() const
{
	return std::holds_alternative<const Terminal*>(*this);
}
Symbol* Symbol::operator->()
{
	return this;
}
template <class T>
bool Symbol::is() const
{
	return std::holds_alternative<T>(*this);
}
template <class T>
T Symbol::as() const
{
	if (!this->is<T>()) throw "Invalid cast";
	return std::get<T>(*this);
}
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


RuleBaseBase::RuleBaseBase(Symbol left, const std::vector<Symbol>& right, const std::string& view):
	left(left),
	right(right),
	dbg_view(view)
{}
RuleBaseBase::RuleBaseBase(const NonTerminal* left, const std::vector<Symbol>& right):
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
/*RuleBaseBase(const std::vector<const Symbol*>& start):
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
RuleBaseBase(const Symbol* terminal):
	left(terminal),
	right()
{
	dbg_view = "["s + terminal->dbg_name + "]"s;
}*/
//RuleBaseBase* RuleBaseBase::makeStart(const NonTerminal* start, const Terminal* end)
//{
//	return new RuleBaseBase(nullptr, { start, end, nullptr }, start->dbg_name);
//}
//static RuleBaseBase* makeStart(const std::vector<const Symbol*>& start)
//{
//	std::string dbg_view = ""s;
//	for (const Symbol* s : start)
//	{
//		dbg_view += s->dbg_name;
//	}
//	return new RuleBaseBase(nullptr, start, dbg_view);
//}
RuleBaseBase* RuleBaseBase::makeChecking(const Terminal* terminal)
{
	//return new RuleBaseBase(terminal, {}, "["s + terminal->token_type.dbg_name + "]"s);
	return new Rule<Token>(terminal);
}

template<class T>
RuleBase<T>::RuleBase(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view):
	RuleBaseBase(left, right, dbg_view)
{}
template<class T>
RuleBase<T>::RuleBase(const NonTerminal* left, const std::vector<Symbol>& right) :
	RuleBaseBase(left, right)
{}

template <class T, class... Childs>
Rule<T, Childs...>::Rule(Symbol left, const std::vector<Symbol>& right, const std::string& view) :
	RuleBase<T>(left, right, view)
{}
template <class T, class... Childs>
Rule<T, Childs...>::Rule(const NonTerminal* left, const std::vector<Symbol>& right, std::function<T(Childs...)> get) :
	RuleBase<T>(left, right),
	get(get)
{}
template<class T, class ...Childs>
TreeNodeBase<T>* Rule<T, Childs...>::make_tree_node(const HistoryState* state) const
{
	return new TreeNode<T, Childs...>(state, std::index_sequence_for<Childs...>());
}
Rule<Token>::Rule(const Terminal* left):
	RuleBase<Token>(left, {}, "["s + left->token_type.dbg_name + "]"s)
{}
TreeNodeBase<Token>* Rule<Token>::make_tree_node(const HistoryState* state) const
{
	return new TreeNode<Token>(state);
}


TreeNodeBaseBase::TreeNodeBaseBase(const HistoryState* state):
	rule(state->state->rule),
	//tokenizer(state->state->tokenizer),
	parent(nullptr)
{}

template<class T>
TreeNodeBase<T>::TreeNodeBase(const HistoryState* state):
	TreeNodeBaseBase(state)
{}
template<class T>
void TreeNodeBase<T>::out(int offset) const
{
	//std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
	//for (const std::unique_ptr<TreeNodeBase>& c : childs)
	//{
	//	c->out(offset + 1);
	//}
}

template<class T, class... Childs>
template<size_t... inds>
TreeNode<T, Childs...>::TreeNode(const HistoryState* state, std::index_sequence<inds...>):
	TreeNodeBase<T>(state),
	//childs(static_cast<const Rule<T, Childs>*>(state->state->rule)->make_tree_node(state->childs[inds].get())...)
	childs(static_cast<const RuleBase<Childs>*>(state->childs[inds]->state->rule)->make_tree_node(state->childs[inds].get())...)
{}

template<class T, class... Childs>
template <std::size_t... Is>
std::tuple<Childs...> TreeNode<T, Childs...>::fill_childs(std::tuple<Childs...>& t, std::index_sequence<Is...>)
{
	//((std::get<Is>(t) = ), ...);
	return std::make_tuple(std::get<Is>(childs)->get()...);
}
template<class T, class... Childs>
std::tuple<Childs...> TreeNode<T, Childs...>::fill_childs(std::tuple<Childs...>& t)
{
	return fill_childs(t, std::index_sequence_for<Childs...>{});
}
template<class T, class... Childs>
T TreeNode<T, Childs...>::get()
{
	std::tuple<Childs...> tuple_childs = fill_childs(tuple_childs);
	return std::apply(static_cast<const Rule<T, Childs...>*>(TreeNodeBaseBase::rule)->get, tuple_childs);
}
//template<class T, class... Ts>
//Tokenizer fill_tree_node_childs(Tokenizer t, std::tuple<TreeNode<T>, TreeNode<Ts>...>& childs)
//{
//	if constexpr (sizeof...(Ts) > 0)
//	{
//		t = head(childs).fill(t);
//		return fill_tree_node_childs(t, tail(childs));
//	}
//}
//template<class T, class ...Childs>
//Tokenizer TreeNode<T, Childs...>::fill(Tokenizer t)
//{
//	return fill_tree_node_childs(t, childs);
//}

TreeNode<Token>::TreeNode(const HistoryState* state):
	TreeNodeBase(state),
	token(state->state->rule->left.as<const Terminal*>()->token_type, state->state->prev->tokenizer.getToken(state->state->rule->left.as<const Terminal*>()->token_type).value().value)
{}
Token TreeNode<Token>::get()
{
	return token;
}
//Tokenizer TreeNode<Token>::fill(Tokenizer t)
//{
//	token = t.getToken(token.type).value();
//	return t.advanced(token.type);
//}


State::SymbolPointer::SymbolPointer(const State* state, std::vector<Symbol>::const_iterator iterator):
	state(state),
	iterator(iterator)
{}
State::SymbolPointer::SymbolPointer(std::nullptr_t):
	state(nullptr)
{}
Symbol State::SymbolPointer::getSymbol() const
{
	return *iterator;
}
Symbol State::SymbolPointer::operator->() const
{
	return *iterator;
}
Symbol State::SymbolPointer::operator*() const
{
	return *iterator;
}

bool operator == (const State::Cycle& c1, const State::Cycle& c2)
{
	return c1.state == c2.state && c1.begin == c2.begin;
}
State::Cycle::Cycle(std::shared_ptr<State> state, std::shared_ptr<State> begin):
	state(state),
	begin(begin)
{}
State::Cycle::Cycle(std::shared_ptr<State> state):
	state(state)
{}

State::State(const RuleBaseBase* rule, std::shared_ptr<const State> prev, SymbolPointer parent):
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

	if (rule->left.isTerminal())
		tokenizer = prev->tokenizer.advanced(rule->left.as<const Terminal*>()->token_type);

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
		for (std::map<const RuleBaseBase*, Cycle>::iterator c = std::begin(cycles); c != std::end(cycles);)
			if (c->second.begin.get() == front_symbol.state)
				c = cycles.erase(c);
			else
				c = std::next(c);
	}
	/*for (Cycle& c : repeated_states)
		if (c.will_be_passed)
			c.passed = true;*/
}
State::State(const RuleBaseBase* start_rule, const std::string& code):
	rule(start_rule),
	prev(nullptr),
	parent(nullptr),
	front_symbol(this, std::begin(start_rule->right)),
	tokenizer(code),
	dbg_hist(),
	dbg_stack({ start_rule->right[0].dbg_name, start_rule->right[1].dbg_name }),
	dbg_depth(0)
{}
bool State::isParentOf(const State& mbChild) const
{
	for (const State* s = &mbChild; s != nullptr; s = s->parent.state)
	{
		if (s == this) return true;
	}
	return false;
}
bool State::isChildOf(const State& mbParent) const
{
	return mbParent.isParentOf(*this);
}
bool State::ruleApplicable(const RuleBaseBase* rule) const
{
	return rule->left == *front_symbol;
}
std::shared_ptr<State> State::advanced(const RuleBaseBase* rule) const
{
	if (!ruleApplicable(rule)) throw "Invalid source stack";
	return std::make_shared<State>(rule, shared_from_this(), front_symbol);
}
std::unique_ptr<HistoryState> State::getTree() const
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
	std::unique_ptr<HistoryState> overRoot = std::make_unique<HistoryState>(statesList.front());
	HistoryState* parentNode = overRoot.get();
	for (const State* state : statesList)
	{
		while (state->parent.state != parent)
		{
			parent = parent->parent.state;
			parentNode = parentNode->parent;
		}
		std::unique_ptr<HistoryState> node = std::make_unique<HistoryState>(state);
		node->parent = parentNode;
		parentNode->childs.push_back(std::move(node));
		parent = state;
		parentNode = parentNode->childs.back().get();
	}
	std::unique_ptr<HistoryState> root = std::move(overRoot->childs.front());
	root->parent = nullptr;
	return root;
}

template<class Start>
class Analyzer
{
	std::map<std::string, Symbol> symbolbyName;

	const Terminal* end_symbol;
	const RuleBaseBase* start_rule;
	std::map<const NonTerminal*, std::list<const RuleBaseBase*>> expandingRules;
	std::map<const Terminal*, RuleBaseBase*> checkingRules;
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
										  [&](const RuleBaseBase* rule) { return leaf->advanced(rule); });*/
				std::shared_ptr<State> current_state = *state;
				state = leafs.erase(state);
				for (const RuleBaseBase* rule : expandingRules[current_state->front_symbol.getSymbol().as<const NonTerminal*>()])
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
	Analyzer(std::list<Symbol> symbols, const NonTerminal* start, std::list<const RuleBaseBase*> rules)
	{
		for (const Symbol& symbol : symbols)
		{
			if (symbol.is<const Terminal*>())
				checkingRules[symbol.as<const Terminal*>()] = RuleBaseBase::makeChecking(symbol.as<const Terminal*>());
		}
		end_symbol = new Terminal(Token::Type::eof);
		checkingRules[end_symbol] = RuleBaseBase::makeChecking(end_symbol);
		//start_rule = RuleBaseBase::makeStart(start, end_symbol);
		start_rule = new Rule<nullptr_t, Start, Token>(nullptr, { start, end_symbol, nullptr }, start->dbg_name);
		for (const RuleBaseBase* rule : rules)
		{
			expandingRules[rule->left.as<const NonTerminal*>()].push_back(rule);
		}
		const RuleBaseBase* start_rule;
	}
	/*Analyzer(const std::list<std::string>& terminals,
			 const std::list<std::string>& nonterminals,
			 const std::string& start,
			 const std::list<std::list<std::string>>& rules)
	{
		for (const std::string& terminal : terminals)
		{
			symbolbyName[terminal] = new Symbol(true, terminal);
			checkingRules[symbolbyName[terminal]] = RuleBaseBase::makeChecking(symbolbyName[terminal]);
		}
		for (const std::string& nonterminal : nonterminals)
		{
			symbolbyName[nonterminal] = new Symbol(false, nonterminal);
		}
		end_symbol = new Symbol(true, "eof");
		symbolbyName["eof"s] = end_symbol;
		checkingRules[end_symbol] = RuleBaseBase::makeChecking(end_symbol);
		start_rule = RuleBaseBase::makeStart(symbolbyName[start], end_symbol);
		for (std::list<std::string> rule : rules)
		{
			const Symbol* left = symbolbyName[rule.front()];
			rule.pop_front();
			std::vector<const Symbol*> right;
			for (const std::string& r : rule)
				right.push_back(symbolbyName[r]);
			expandingRules[left].push_back(new RuleBaseBase(left, right));
		}
	}*/

	TreeNodeBase<Start>* analyze(const std::string& code)
	{
		std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule, code) };
		for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
		{
			std::list<std::shared_ptr<State>> expanded_leafs = expand(*leaf);
			for (const std::shared_ptr<State>& expanded_leaf : expanded_leafs)
			{
				const Terminal* front_symbol = expanded_leaf->front_symbol->as<const Terminal*>();
				bool has_value = expanded_leaf->tokenizer.checkToken(front_symbol->token_type);
				if (has_value)
				{
					std::shared_ptr<State> advanced_leaf = expanded_leaf->advanced(checkingRules[front_symbol]);
					if (front_symbol == end_symbol)
					{
						leafs.push_front(advanced_leaf);
					}
					else
						leafs.push_back(advanced_leaf);

					std::map<const RuleBaseBase*, State::Cycle> passed_cycles;
					std::map<const RuleBaseBase*, State::Cycle>::iterator exp_it = std::begin(expanded_leaf->cycles);
					std::map<const RuleBaseBase*, State::Cycle>::iterator adv_it = std::begin(advanced_leaf->cycles);
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
					for (const std::pair<const RuleBaseBase*, State::Cycle>& c : passed_cycles)
					{
						leafs.push_back(c.second.state);
						leafs.back()->cycles.insert_or_assign(c.first, c.second);
					}
				}
			}
		}
		if (leafs.size() == 0) throw "Code is invalid";
		else if (leafs.size() > 1) throw "Code is ambiguous";
		else
		{
			std::unique_ptr<HistoryState> hist = leafs.front()->getTree();
			TreeNode<nullptr_t, Start, Token>* start_node = new TreeNode<nullptr_t, Start, Token>(hist.get(), std::make_index_sequence<2>());
			return std::get<0>(start_node->childs);
		}
	}
};

int main()
{
	/*Token::Type ta("a"s, std::regex("a"));
	Token::Type tb("b"s, std::regex("b"));

	const Terminal* sa = new Terminal(ta);
	const Terminal* sb = new Terminal(tb);
	const NonTerminal* sA = new NonTerminal("A"s);
	const NonTerminal* sB = new NonTerminal("B"s);

	std::list<const RuleBaseBase*> rules
	{	new RuleBaseBase(sA, { sa, sB }),
		new RuleBaseBase(sB, { }),
		new RuleBaseBase(sA, { }),
		new RuleBaseBase(sB, { sb, sA }),
	};
	Analyzer a({ sa,sb,sA,sB }, sA, rules);
	std::unique_ptr<TreeNodeBase> root = a.analyze("ababababab"s);
	root->out(0);*/

	std::map<std::string, double> variables;
	double pi = 3.14;
	double e = 2.71;
	double qwe = 42;
	variables["pi"s] = pi;
	variables["e"s] = e;
	variables["qwe"s] = qwe;

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

	//std::list<const RuleBaseBase*> rules
	//{	new RuleBaseBase(s_expression, { s_sum }),
	//	new RuleBaseBase(s_sum, { s_mul }),
	//	new RuleBaseBase(s_sum, { s_mul, s_oper_plus, s_sum }),
	//	new RuleBaseBase(s_val, { s_oper_lb, s_sum, s_oper_rb }),
	//	new RuleBaseBase(s_mul, { s_val }),
	//	new RuleBaseBase(s_mul, { s_val, s_oper_mul, s_mul }),
	//	new RuleBaseBase(s_val, { s_variable }),
	//	new RuleBaseBase(s_val, { s_integer }),
	//};
	auto forward = [](auto v) { return v; };
	std::list<RuleBaseBase*> rules
	{	new Rule<double, double>(s_expression, { s_sum }, forward),
		new Rule<double, double>(s_sum, { s_mul }, forward),
		new Rule<double, double, Token, double>(s_sum, { s_mul, s_oper_plus, s_sum }, [](double d1, Token, double d2) { return d1 + d2; }),
		new Rule<double, Token, double, Token>(s_val, { s_oper_lb, s_sum, s_oper_rb }, [](Token, double d, Token) { return d; }),
		new Rule<double, double>(s_mul, { s_val }, forward),
		new Rule<double, double, Token, double>(s_mul, { s_val, s_oper_mul, s_mul }, [](double d1, Token, double d2) { return d1 * d2; }),
		new Rule<double, Token>(s_val, { s_variable }, [&](Token t) { return variables[t.value]; }),
		new Rule<double, Token>(s_val, { s_integer }, [&](Token t)
			{
				return std::stod(t.value);
			}),
	};
	Analyzer<double> analyzer({ s_variable,s_integer,s_oper_mul,s_oper_div,s_oper_plus,s_oper_minus,s_oper_eq,s_oper_lb,s_oper_rb,s_expression,s_val,s_sum,s_mul }, s_expression, std::list<const RuleBaseBase*>(rules.cbegin(), rules.cend()));
	std::string code = "qwe*(pi+123)+34"s;
	TreeNodeBase<double>* root = analyzer.analyze(code);
	root->out(0);
	std::cout << root->get() << "   " << qwe * (pi + 123) + 34;



	std::list<RuleBaseBase*>::iterator it_rule = rules.begin();
	//static_cast<Rule<double>*>(*it_rule++)->get = [&](double d) { return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]) + static_cast<const Rule<double>*>(node.childs[2]->rule)->getb(*node.childs[2]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[1]->rule)->getb(*node.childs[1]); return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]) * static_cast<const Rule<double>*>(node.childs[2]->rule)->getb(*node.childs[2]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node)
	//{
	//	double d = variables[node.childs[0]->parent->tokenizer.getToken(variable).value().value];  return d;
	//};
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node)
	//{
	//	std::optional<Token> t = node.childs[0]->parent->tokenizer.getToken(integer);
	//	std::string s = t.value().value;
	//	double d = std::stod(s);
	//	//std::cout << d << std::endl;
	//	return d;
	//};

	//std::list<RuleBaseBase*>::iterator it_rule = rules.begin();
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]) + static_cast<const Rule<double>*>(node.childs[2]->rule)->getb(*node.childs[2]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[1]->rule)->getb(*node.childs[1]); return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node) { double d = static_cast<const Rule<double>*>(node.childs[0]->rule)->getb(*node.childs[0]) * static_cast<const Rule<double>*>(node.childs[2]->rule)->getb(*node.childs[2]);  return d; };
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node)
	//{
	//	double d = variables[node.childs[0]->parent->tokenizer.getToken(variable).value().value];  return d;
	//};
	//static_cast<Rule<double>*>(*it_rule++)->getb = [&](const TreeNodeBase& node)
	//{
	//	std::optional<Token> t = node.childs[0]->parent->tokenizer.getToken(integer);
	//	std::string s = t.value().value;
	//	double d = std::stod(s);
	//	//std::cout << d << std::endl;
	//	return d;
	//};
	//std::cout << static_cast<const Rule<double>*>(rules.front())->getb(*root) << "   " << qwe * (pi + 123) + 34;



	/*std::unique_ptr<TreeNodeBase> root = Analyzer(
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
	std::list<const RuleBaseBase*> rules1
	{	new RuleBaseBase(s_A, { s_B, s_a }),
		new RuleBaseBase(s_B, { s_A, s_b }),
		new RuleBaseBase(s_A, { }),
		new RuleBaseBase(s_B, { }),
	};
	Analyzer analyzer1({ s_a, s_b, s_A, s_B }, s_A, rules1);
	std::unique_ptr<TreeNodeBase> root1 = analyzer1.analyze("abababa");
	root1->out(0);*/
	return 0;
}
