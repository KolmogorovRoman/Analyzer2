#include "AnalyzerBase.h"

Terminal::Terminal(const Token::Type* token_type):
	token_type(token_type)
{}

NonTerminal::NonTerminal(std::string dbg_name):
	dbg_name(dbg_name)
{}

Symbol::Symbol():
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>((const NonTerminal*)nullptr),
	dbg_name(""s)
{}
Symbol::Symbol(nullptr_t):
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>((const NonTerminal*)nullptr),
	dbg_name(""s)
{}
Symbol::Symbol(const Terminal* terminal):
	std::variant</*nullptr_t,*/ const Terminal*, const NonTerminal*>(terminal),
	dbg_name(terminal->token_type->dbg_name)
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
bool Symbol::operator==(const Symbol& other) const
{
	return std::operator==(*this, other);
}
const Terminal* Symbol::eof_v = nullptr;
const Terminal* Symbol::empty_v = nullptr;
const Terminal* Symbol::eof()
{
	if (eof_v == nullptr)
		eof_v = new Terminal(&Token::Type::eof);
	return eof_v;
}
const Terminal* Symbol::empty()
{
	if (empty_v == nullptr)
		empty_v = new Terminal(&Token::Type::empty);
	return empty_v;
}

const Terminal* Symbol::asTerminal() const
{
	return this->as<const Terminal*>();
}

const NonTerminal* Symbol::asNonTerminal() const
{
	return this->as<const NonTerminal*>();
}


//State::SymbolPointer::SymbolPointer(const State* state, std::vector<Symbol>::const_iterator iterator):
//	state(state),
//	iterator(iterator)
//{}
//State::SymbolPointer::SymbolPointer(const RuleNode* ruleNode, std::vector<Symbol>::const_iterator iterator):
//	ruleNode(ruleNode),
//	iterator(iterator)
//{}
//State::SymbolPointer::SymbolPointer(std::nullptr_t):
//	state(nullptr)
//{}
//Symbol State::SymbolPointer::getSymbol() const
//{
//	return *iterator;
//}
//Symbol State::SymbolPointer::operator->() const
//{
//	return *iterator;
//}
//Symbol State::SymbolPointer::operator*() const
//{
//	return *iterator;
//}

HistoryState::HistoryState(std::shared_ptr<const State> state):
	//rule(state->ruleNode->rule),
	tokenizer(state->tokenizer)
{}
void HistoryState::out(int offset) const
{
	std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
	for (const std::unique_ptr<HistoryState>& s : childs)
		s->out(offset + 1);
}

//bool operator == (const State::Cycle& c1, const State::Cycle& c2)
//{
//	return c1.state == c2.state && c1.begin == c2.begin;
//}
//State::Cycle::Cycle(std::shared_ptr<State> state, std::shared_ptr<State> begin):
//	state(state),
//	begin(begin)
//{}
//State::Cycle::Cycle(std::shared_ptr<State> state):
//	state(state)
//{}

RuleNode::RuleNode():
	rule(nullptr)
{}
RuleNode::RuleNode(const Rule0* rule, Symbol symbol):
	rule(rule),
	symbol(symbol)
{}

bool RuleNode::isNulling() const
{
	return symbol == Symbol::empty();
}

bool RuleNode::isChecking() const
{
	return rule != nullptr && rule->left.isTerminal();
}

bool RuleNode::isChecking(const Terminal* mbTerminal) const
{
	return isChecking() && rule->left == mbTerminal;
}

//State::Cycle::Cycle(const State* start, std::shared_ptr<const State> state):
//	start(start), state(state)
//{}

//State::State(const Rule0* rule, std::shared_ptr<const State> prev, const State* parent):
//	prev(prev),
//	parent(parent),
//	tokenizer(ruleNode->symbol.isTerminal() ?
//		prev->tokenizer.advanced(ruleNode->symbol.as<const Terminal*>()->token_type) :
//		prev->tokenizer)
//{
//	//dbg_hist = prev->dbg_hist;
//	//dbg_hist.push_back(prev.get());
//	//dbg_stack = parent->dbg_stack;
//	//dbg_stack.pop_front();
//	//std::list<std::string>::iterator b = dbg_stack.begin();
//	//for (const Symbol& s : rule->right)
//	//	dbg_stack.insert(b, s.dbg_name);
//	//dbg_depth = parent->dbg_depth + 1;
//
//	//if (rule->left.isTerminal())
//	//	tokenizer = prev->tokenizer.advanced(rule->left.as<const Terminal*>()->token_type);
//
//	//while (front_symbol.iterator == std::end(front_symbol.state->ruleNode->rule->right))
//	//while (front_symbol->ruleNode->rule->right.empty())
//	//{
//	//	if (front_symbol.state == nullptr) throw "Stack is empty";
//	//	front_symbol = SymbolPointer(front_symbol.state->parent.state, std::next(front_symbol.state->parent.iterator));
//	//}
//}
State::State(const RuleNode* ruleNode, const Tokenizer& tokenizer, std::shared_ptr<const State> prev, const State* parent, const State* cycle_start, const State* cycle_end):
	ruleNode(ruleNode),
	prev(prev),
	parent(parent),
	cycle_start(cycle_start != nullptr ? cycle_start : this),
	cycle_end(cycle_start == nullptr ? cycle_end : this),
	tokenizer(tokenizer),
	dbg_depth(parent != nullptr ? parent->dbg_depth + 1 : 0)
{
	for (const State* p = this; p != nullptr; p = p->parent)
		dbg_hist.push_front(p);
}
std::shared_ptr<const State> State::make(const RuleNode* ruleNode, std::shared_ptr<const State> prev, const State* parent, const State* cycle_end)
{
	if (!ruleNode->symbol.isTerminal() || prev->tokenizer.checkToken(ruleNode->symbol.asTerminal()->token_type))
	{
		const State* cycle_start = nullptr;
		const State* s = parent;
		Tokenizer tokenizer = ruleNode->isChecking() ?
			prev->tokenizer.advanced(parent->ruleNode->symbol.asTerminal()->token_type) :
			prev->tokenizer;
		while (s != nullptr && s->tokenizer.head == tokenizer.head)
		{
			if (s->ruleNode->symbol == ruleNode->symbol)
			{
				cycle_start = s;
			}
			s = s->parent;
		}
		//if (cycle_start == nullptr) { cycle_start = parent; cycle_end = parent; }
		return std::make_shared<State>(ruleNode, tokenizer, prev, parent, cycle_start, cycle_end);
	}
	else return nullptr;
}
State::State(const RuleNode* startRuleNode, const std::string& code):
	prev(nullptr),
	parent(nullptr),
	cycle_start(this),
	cycle_end(this),
	tokenizer(code),
	ruleNode(startRuleNode),
	dbg_depth(0)
{}
bool State::isParentOf(const State& mbChild) const
{
	for (const State* s = mbChild.parent; s != nullptr; s = s->parent)
	{
		if (s == this) return true;
	}
	return false;
}
bool State::isChildOf(const State& mbParent) const
{
	return mbParent.isParentOf(*this);
}
std::pair<std::list<std::shared_ptr<const State>>, std::list<std::shared_ptr<const State>>> State::expanded(const AnalyzerBase* analyzer) const
{
	std::pair<std::list<std::shared_ptr<const State>>, std::list<std::shared_ptr<const State>>> states;
	states.first.push_back(shared_from_this());
	for (std::list<std::shared_ptr<const State>>::iterator state = std::begin(states.first); state != std::end(states.first); state = states.first.erase(state))
	{
		//if (state->get()->ruleNode->symbol == analyzer->empty_symbol != state->get()->ruleNode->isChecking())
		//{
		//	state = state;
		//}
		if (state->get()->ruleNode->isNulling())
		{
			//if (!state->get()->parent->ruleNode->symbol.isTerminal())
			//{
			//	states.first.insert_range(state, state->get()->continued(analyzer));
			//}
			//else
			states.first.insert(state, *state);
		}
		else
			for (const std::pair<const Symbol, RuleNode>& sr : analyzer->rootRuleNodes.at(state->get()->ruleNode->symbol).continuations)
			{
				std::shared_ptr<const State> result = State::make(&sr.second, *state, state->get(), state->get());
				if (result != nullptr)
				{
					if (result->cycle_start == result.get())
						states.first.push_back(result);
					else
						states.second.push_back(result);
				}
			}
	}
	return states;
}
std::list<std::shared_ptr<const State>> State::continued(const AnalyzerBase* analyzer) const
{
	std::list<std::shared_ptr<const State>> states;
	const State* start = this;
	const State* state = this;
	const State* cycle_end = this->cycle_end;
	while (state != nullptr)
	{
		//if (state->isParentOf(*start))
		{
			if (cycle_end != nullptr && cycle_end->cycle_start == state)
			{
				states.append_range(cycle_end->expanded(analyzer).second);
				cycle_end = state->cycle_end;
			}
			for (const std::pair<const Symbol, RuleNode>& sr : state->ruleNode->continuations)
			{
				std::shared_ptr<const State> result = State::make(&sr.second, this->shared_from_this(), state->parent, cycle_end);
				if (result != nullptr && result->cycle_start == result.get())
					states.push_back(result);
			}
			if (state->ruleNode->rule != nullptr)
			{
				start = state;
			}
			else
				break;
		}
		state = state->parent;
		//state = state->prev.get();
	}
	return states;
}
std::list<std::shared_ptr<const State>> State::getHistList() const
{
	std::list<std::shared_ptr<const State>> statesList;
	std::shared_ptr<const State> state = shared_from_this();
	while (state != nullptr)
	{
		//if (!(state->parent!=nullptr && state->parent->ruleNode->symbol.isTerminal()))
		statesList.push_front(state);
		state = state->prev;
	}
	return statesList;
}
std::unique_ptr<HistoryState> State::getTree() const
{
	std::list<std::shared_ptr<const State>> statesList;
	std::shared_ptr<const State> state = shared_from_this();
	while (state != nullptr)
	{
		//if (!(state->parent!=nullptr && state->parent->ruleNode->symbol.isTerminal()))
		statesList.push_front(state);
		state = state->prev;
	}

	//statesList.pop_front();
	std::unique_ptr<HistoryState> overRoot = std::make_unique<HistoryState>(statesList.front());
	overRoot->parent = nullptr;
	HistoryState* parentNode = overRoot.get();
	const State* parent = nullptr;
	for (std::shared_ptr<const State> state : statesList)
	{
		while (state->parent != parent)
		{
			parent = parent->parent;
			parentNode = parentNode->parent;
		}
		if (state->ruleNode->rule != nullptr) parentNode->rule = state->ruleNode->rule;
		if (state->ruleNode->isNulling())
			continue;
		std::unique_ptr<HistoryState> node = std::make_unique<HistoryState>(state);
		node->parent = parentNode;
		parentNode->childs.push_back(std::move(node));
		parent = state.get();
		parentNode = parentNode->childs.back().get();
	}
	//std::unique_ptr<HistoryState> root = std::move(overRoot->childs.front());
	//root->parent = nullptr;
	//return std::move(root);
	return std::move(overRoot);
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
	Rule1<Token>(left, {}, "["s + left->token_type->dbg_name + "]"s)
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
	rule(state->rule),
	//tokenizer(state->tokenizer),
	parent(nullptr)
{}

TreeNode2<Token>::TreeNode2(const HistoryState* state):
	TreeNode1(state),
	token(*state->rule->left.as<const Terminal*>()->token_type, state->tokenizer.getToken(state->rule->left.as<const Terminal*>()->token_type).value().value)
{}
void TreeNode2<Token>::out(int offset) const
{
	std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
}
Token TreeNode2<Token>::get()
{
	return token;
}



AnalyzerBase::AnalyzerBase(std::list<Symbol> symbols, /*const Rule0* start_rule, std::map<const Terminal*, Rule0*> checkingRules,*/ std::list<const Rule0*> rules)
	//start_rule(start_rule),
	//empty_symbol(new Terminal(&Token::Type::empty)),
	//end_symbol(new Terminal(&Token::Type::eof))
	//checkingRules(checkingRules)
{
	for (const Rule0* rule : rules)
	{
		expandingRules[rule->left.as<const NonTerminal*>()].push_back(rule);

		RuleNode* rn = &rootRuleNodes[rule->left];
		rn->symbol = rule->left;
		std::string dbg_view = rule->left.dbg_name + " -> "s;
		if (rule->right.empty())
		{
			rn = &rn->continuations[Symbol::empty()];
			rn->symbol = Symbol::empty();
			//rn->dbg_view = "[" + rule->left.dbg_name + "] "s;
		}
		else
		{
			for (std::vector<Symbol>::const_iterator s = rule->right.cbegin(); s != rule->right.cend() - 1; s++)
			{
				rn = &rn->continuations[*s];
				rn->symbol = *s;
				dbg_view += s->dbg_name + " "s;
				//rn->dbg_view = dbg_view + "..."s;
			}
			rn = &rn->continuations[rule->right.back()];
			rn->symbol = rule->right.back();
			dbg_view += "[" + rule->right.back().dbg_name + "] "s;
			//rn->dbg_view = dbg_view;
		}
		rn->rule = rule;
	}

	for (const Symbol& symbol : symbols)
	{
		if (symbol.isTerminal())
		{
			checkingRules[symbol.asTerminal()] = Rule2<Token>::makeChecking(symbol.asTerminal());
			RuleNode* rn = &rootRuleNodes[symbol];
			rn->symbol = symbol;
			rn = &rn->continuations[Symbol::empty()];
			rn->symbol = Symbol::empty();
			rn->rule = checkingRules.at(symbol.asTerminal());
			//rn->dbg_view = "["s + symbol.dbg_name + "]"s;
		}
	}
	checkingRules[Symbol::eof()] = Rule2<Token>::makeChecking(Symbol::eof());
	rootRuleNodes[Symbol::eof()].symbol = Symbol::eof();
	rootRuleNodes[Symbol::eof()].continuations[Symbol::empty()].symbol = Symbol::empty();
	rootRuleNodes[Symbol::eof()].continuations[Symbol::empty()].rule = checkingRules[Symbol::eof()];


	std::list<RuleNode*> rns;
	std::map<RuleNode*, std::string> views;
	for (std::pair<const Symbol, RuleNode>& rn : rootRuleNodes)
	{
		views[&rn.second] = rn.first.dbg_name + " -> ";
		rns.push_back(&rn.second);
	}
	for (std::list<RuleNode*>::iterator rn = rns.begin(); rn != rns.end(); rn = rns.erase(rn))
	{
		for (std::pair<const Symbol, RuleNode>& cn : (*rn)->continuations)
		{
			//rns.push_back(&cn.second);
			rns.insert(std::next(rn), &cn.second);
			views[&cn.second] = views[*rn] + cn.first.dbg_name + " ";
			if ((*rn)->symbol.isTerminal() && cn.first.isTerminal() && cn.first.asTerminal() == Symbol::empty())
				cn.second.dbg_view = "[" + (*rn)->symbol.dbg_name + "]";
			else
				cn.second.dbg_view = views[*rn] + "(" + cn.first.dbg_name + ") ";
		}
		RuleNode* pn = *rn;
		while (pn->continuations.size() == 1)
		{
			pn = &pn->continuations.begin()->second;
			(*rn)->dbg_view += pn->symbol.dbg_name + " ";
		}
		if (!pn->continuations.empty()) (*rn)->dbg_view += "...[" + std::to_string(pn->continuations.size()) + "]";
	}
}
std::unique_ptr<HistoryState> AnalyzerBase::analyze(const std::string& code, bool dbg_out)
{
	std::list<std::shared_ptr<const State>> leafs = { std::make_shared<State>(startRuleNode, code) };
	for (std::list<std::shared_ptr<const State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
	{
		//if (dbg_out)
		//	std::cout << leaf->get()->dbg_depth << '\t'
		//	<< leaf->get()->cycle_start << '\t'
		//	<< leaf->get()->cycle_end << '\t'
		//	<< (leaf->get()->ruleNode->rule != nullptr ? leaf->get()->ruleNode->rule->dbg_view : leaf->get()->ruleNode->symbol.dbg_name) << '\t'
		//	<< std::string_view(leaf->get()->tokenizer.head, leaf->get()->tokenizer.code->cend()) << '\t'
		//	<< std::endl;
		if (leaf->get()->ruleNode->isChecking(Symbol::eof()))
			leafs.insert(leaf, *leaf);
		else if (leaf->get()->ruleNode->symbol == Symbol::empty())
			leafs.append_range(leaf->get()->continued(this));
		else
			leafs.append_range(leaf->get()->expanded(this).first);
		if (leafs.size() == 1 /*&& dbg_out*/)
		{
			std::list<std::shared_ptr<const State>> hist_list = leafs.front()->getHistList();
			//std::unique_ptr<HistoryState> hist = leafs.front()->getTree();
			//hist->out(0);
			for (std::shared_ptr<const State> s : hist_list)
			{
				std::cout << s->dbg_depth << '\t' << s->ruleNode->dbg_view << std::endl;
			}
			std::cout << std::string_view(leaf->get()->tokenizer.code->cbegin(), leaf->get()->tokenizer.head);
			std::cout << ">>>ERROR<<<";
			std::cout << std::string_view(leaf->get()->tokenizer.head, leaf->get()->tokenizer.code->cend()) << std::endl;
			std::cout << "Excepted one of:" << std::endl;
			for (const std::pair<const Symbol, RuleNode>& p : leafs.back()->ruleNode->continuations)
				std::cout << p.first.dbg_name << std::endl;
		} //ERROR
	}
	if (leafs.size() == 0) throw "Code is invalid";
	//else if (leafs.size() > 1)
	//{
	//	for (shared_ptr<const State> s : leafs)
	//	{
	//		s->getTree()->out(0);
	//	}
	//	throw "Code is ambiguous";
	//}
	else
	{
		std::unique_ptr<HistoryState> hist = leafs.front()->getTree();
		return hist;
	}
}

