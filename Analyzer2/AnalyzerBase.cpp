#include "AnalyzerBase.h"

Terminal::Terminal(Token::Type token_type):
	token_type(token_type)
{}

NonTerminal::NonTerminal(std::string dbg_name):
	dbg_name(dbg_name)
{}

Symbol::Symbol(nullptr_t):
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>((const NonTerminal*)nullptr),
	dbg_name(""s)
{}
Symbol::Symbol(const Terminal* terminal):
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>(terminal),
	dbg_name(terminal->token_type.dbg_name)
{}
Symbol::Symbol(const NonTerminal* non_terminal):
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>(non_terminal),
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

HistoryState::HistoryState(std::shared_ptr<const State> state):
	state(state)
{}
void HistoryState::out(int offset) const
{
	std::cout << std::string(offset, ' ') << state->rule->dbg_view << std::endl;
	for (const std::unique_ptr<HistoryState>& s : childs)
		s->out(offset + 1);
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

State::State(const Rule0* rule, std::shared_ptr<const State> prev, SymbolPointer parent):
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
		//for (std::map<const Rule0*, Cycle>::iterator c = std::begin(cycles); c != std::end(cycles);)
		//	if (c->second.begin.get() == front_symbol.state)
		//		c = cycles.erase(c);
		//	else
		//		c = std::next(c);
	}
	/*for (Cycle& c : repeated_states)
		if (c.will_be_passed)
			c.passed = true;*/
}
State::State(const Rule0* start_rule, const std::string& code):
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
bool State::ruleApplicable(const Rule0* rule) const
{
	return rule->left == *front_symbol;
}
std::shared_ptr<State> State::advanced(const Rule0* rule) const
{
	if (!ruleApplicable(rule)) throw "Invalid source stack";
	return std::make_shared<State>(rule, shared_from_this(), front_symbol);
}
std::unique_ptr<HistoryState> State::getTree() const
{
	std::list<std::shared_ptr<const State>> statesList;
	std::shared_ptr<const State> state = shared_from_this();
	while (state != nullptr)
	{
		statesList.push_front(state);
		state = state->prev;
	}

	const State* parent = nullptr;
	//statesList.pop_front();
	std::unique_ptr<HistoryState> overRoot = std::make_unique<HistoryState>(statesList.front());
	HistoryState* parentNode = overRoot.get();
	for (std::shared_ptr<const State> state : statesList)
	{
		while (state->parent.state != parent)
		{
			parent = parent->parent.state;
			parentNode = parentNode->parent;
		}
		std::unique_ptr<HistoryState> node = std::make_unique<HistoryState>(std::shared_ptr<const State>(state));
		node->parent = parentNode;
		parentNode->childs.push_back(std::move(node));
		parent = state.get();
		parentNode = parentNode->childs.back().get();
	}
	std::unique_ptr<HistoryState> root = std::move(overRoot->childs.front());
	root->parent = nullptr;
	return std::move(root);
}


Rule0::Rule0(Symbol left, const std::vector<Symbol>& right, const std::string& view):
	left(left),
	right(right),
	dbg_view(view)
{}
Rule0::Rule0(const NonTerminal* left, const std::vector<Symbol>& right):
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

Rule2<Token>::Rule2(const Terminal* left):
	Rule1<Token>(left, {}, "["s + left->token_type.dbg_name + "]"s)
{}
TreeNode1<Token>* Rule2<Token>::make_tree_node(const HistoryState* state) const
{
	return new TreeNode2<Token>(state);
}
Rule0* Rule2<Token>::makeChecking(const Terminal* terminal)
{
	return new Rule2<Token>(terminal);
}

TreeNode0::TreeNode0(const HistoryState* state):
	rule(state->state->rule),
	//tokenizer(state->state->tokenizer),
	parent(nullptr)
{}

TreeNode2<Token>::TreeNode2(const HistoryState* state):
	TreeNode1(state),
	token(state->state->rule->left.as<const Terminal*>()->token_type, state->state->prev->tokenizer.getToken(state->state->rule->left.as<const Terminal*>()->token_type).value().value)
{}
void TreeNode2<Token>::out(int offset) const
{
	std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
}
Token TreeNode2<Token>::get()
{
	return token;
}



std::list<std::shared_ptr<State>> AnalyzerBase::expand(const std::shared_ptr<State>& leaf)
{
	std::list<std::shared_ptr<State>> leafs({ leaf });
	std::list<std::shared_ptr<State>> repeated_states;
	for (std::list<std::shared_ptr<State>>::iterator state = std::begin(leafs); state != std::end(leafs);)
	{
		if (state->get()->front_symbol->isTerminal())
		{
			state++;
			continue;
		}
		else
		{
			std::shared_ptr<State> current_state = *state;
			state = leafs.erase(state);
			for (const Rule0* rule : expandingRules[current_state->front_symbol.getSymbol().as<const NonTerminal*>()])
			{
				//std::shared_ptr<State> new_state = current_state->advanced(rule);
				//state = leafs.insert(state, new_state);

				std::shared_ptr<State> new_state = current_state->advanced(rule);
				//bool stateRepeats = false;
				//for (const State* l = current_state.get(); l != leaf->front_symbol.state; l = l->parent.state)
				//{
				//	if (new_state->rule == l->rule)
				//	{
				//		stateRepeats = true;
				//		break;
				//	}
				//}
				//if (stateRepeats)
				//{
				//	repeated_states.push_back(new_state);
				//}
				//else
				{
					state = leafs.insert(state, new_state);
				}

				/*std::shared_ptr<State> new_state = current_state->advanced(rule);
				if (new_state->rule == current_state->rule)
					continue;
				else
					state = leafs.insert(state, new_state);*/
			}
		}
	}
	//for (const std::shared_ptr<State>& l : leafs)
	//{
	//	for (const std::shared_ptr<State>& c : repeated_states)
	//	{
	//		bool cycle_added = l->cycles.insert(std::make_pair(c->rule, State::Cycle(c, leaf))).second;
	//		if (!cycle_added) l->cycles.at(c->rule).state = c;
	//	}
	//}
	return leafs;
}
AnalyzerBase::AnalyzerBase(std::list<Symbol> symbols, /*const Rule0* start_rule, std::map<const Terminal*, Rule0*> checkingRules,*/ std::list<const Rule0*> rules):
	start_rule(start_rule),
	end_symbol(new Terminal(Token::Type::eof))
	//checkingRules(checkingRules)
{
	for (const Rule0* rule : rules)
	{
		expandingRules[rule->left.as<const NonTerminal*>()].push_back(rule);

		RuleNode* rn = &rootRuleNodes[rule->left];
		for (const Symbol& s : rule->right)
		{
			rn = &rn->childs[s];
		}
		rn->rule = rule;
	}
}
std::unique_ptr<HistoryState> AnalyzerBase::analyze(const std::string& code)
{
	std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule, code) };
	for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
	{
		std::cout << leafs.size() << " ";
		std::list<std::shared_ptr<State>> expanded_leafs = expand(*leaf);
		if (expanded_leafs.size() > 100) 
			std::cout << expanded_leafs.size() << " ";
		std::cout << leaf->get()->dbg_stack.size();
		std::cout << std::endl;
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

				//std::map<const Rule0*, State::Cycle> passed_cycles;
				//std::map<const Rule0*, State::Cycle>::iterator exp_it = std::begin(expanded_leaf->cycles);
				//std::map<const Rule0*, State::Cycle>::iterator adv_it = std::begin(advanced_leaf->cycles);
				//while (exp_it != std::end(expanded_leaf->cycles))
				//{
				//	if (adv_it != std::end(advanced_leaf->cycles) && *exp_it == *adv_it)
				//	{
				//		exp_it = std::next(exp_it);
				//		adv_it = std::next(adv_it);
				//	}
				//	else
				//	{
				//		passed_cycles.insert(*exp_it);
				//		exp_it = std::next(exp_it);
				//	}
				//}
				//for (const std::pair<const Rule0*, State::Cycle>& c : passed_cycles)
				//{
				//	leafs.push_back(c.second.state);
				//	leafs.back()->cycles.insert_or_assign(c.first, c.second);
				//}
			}
		}
		if (leafs.size() == 1)
		{
			//leafs.front()->getTree()->out(0);
			std::cout << std::string_view(leaf->get()->tokenizer.code->cbegin(), leaf->get()->tokenizer.head);
			std::cout << ">>>ERROR<<<";
			std::cout << std::string_view(leaf->get()->tokenizer.head, leaf->get()->tokenizer.code->cend()) << std::endl;
			std::cout << "Excepted one of:" << std::endl;
			for (const std::shared_ptr<State>& expanded_leaf : expanded_leafs)
				std::cout << expanded_leaf->front_symbol->dbg_name << std::endl;
		}
	}
	if (leafs.size() == 0) throw "Code is invalid";
	else if (leafs.size() > 1)
	{
		for (shared_ptr<State> s : leafs)
		{
			s->getTree()->out(0);
		}
		throw "Code is ambiguous";
	}
	else
	{
		std::unique_ptr<HistoryState> hist = leafs.front()->getTree();
		return hist;
	}
}