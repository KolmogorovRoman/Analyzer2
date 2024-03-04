#include "AnalyzerBase.h"

Terminal::Terminal(Token::Type token_type):
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
bool Symbol::operator==(const Symbol& other) const
{
	return std::operator==(*this, other);
}
//Symbol Symbol::empty(new Terminal(Token::Type::empty));
//Symbol Symbol::eof(new Terminal(Token::Type::eof));

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


State::State(const Rule0* rule, std::shared_ptr<const State> prev, const State* parent):
	prev(prev),
	parent(parent),
	tokenizer(ruleNode->symbol.isTerminal() ?
		prev->tokenizer.advanced(ruleNode->symbol.as<const Terminal*>()->token_type) :
		prev->tokenizer)
{
	//dbg_hist = prev->dbg_hist;
	//dbg_hist.push_back(prev.get());
	//dbg_stack = parent->dbg_stack;
	//dbg_stack.pop_front();
	//std::list<std::string>::iterator b = dbg_stack.begin();
	//for (const Symbol& s : rule->right)
	//	dbg_stack.insert(b, s.dbg_name);
	//dbg_depth = parent->dbg_depth + 1;

	//if (rule->left.isTerminal())
	//	tokenizer = prev->tokenizer.advanced(rule->left.as<const Terminal*>()->token_type);

	//while (front_symbol.iterator == std::end(front_symbol.state->ruleNode->rule->right))
	//while (front_symbol->ruleNode->rule->right.empty())
	//{
	//	if (front_symbol.state == nullptr) throw "Stack is empty";
	//	front_symbol = SymbolPointer(front_symbol.state->parent.state, std::next(front_symbol.state->parent.iterator));
	//}
}
State::State(const RuleNode* ruleNode, std::shared_ptr<const State> prev, const State* parent):
	ruleNode(ruleNode),
	prev(prev),
	parent(parent),
	tokenizer((parent != nullptr && parent->ruleNode->symbol.isTerminal()) ?
		prev->tokenizer.advanced(parent->ruleNode->symbol.asTerminal()->token_type) :
		prev->tokenizer)
{
	//dbg_hist = prev->dbg_hist;
	//dbg_hist.push_back(prev.get());
	//dbg_stack = parent->dbg_stack;
	////dbg_stack.pop_front();
	//std::list<std::string>::iterator b = dbg_stack.begin();
	//for (const Symbol& s : ruleNode->rule->right)
	//	dbg_stack.insert(b, s.dbg_name);
	//dbg_depth = parent->dbg_depth + 1;

	//if (ruleNode->rule->left.isTerminal())
	//	tokenizer = prev->tokenizer.advanced(ruleNode->rule->left.as<const Terminal*>()->token_type);
}
std::shared_ptr<const State> State::make(const RuleNode* ruleNode, std::shared_ptr<const State> prev, const State* parent)
{
	if (!ruleNode->symbol.isTerminal() || prev->tokenizer.checkToken(ruleNode->symbol.asTerminal()->token_type))
		return std::make_shared<State>(ruleNode, prev, parent);
	else return nullptr;
}
State::State(const RuleNode* startRuleNode, const std::string& code):
	prev(nullptr),
	parent(nullptr),
	tokenizer(code),
	ruleNode(startRuleNode)
	//dbg_hist(),
	//dbg_stack({ startRuleNode->rule->right[0].dbg_name, startRuleNode->rule->right[1].dbg_name }),
	//dbg_depth(0)
{}
bool State::isParentOf(const State& mbChild) const
{
	for (const State* s = &mbChild; s != nullptr; s = s->parent)
	{
		if (s == this) return true;
	}
	return false;
}
bool State::isChildOf(const State& mbParent) const
{
	return mbParent.isParentOf(*this);
}
std::list<std::shared_ptr<const State>> State::expanded(const std::map<Symbol, RuleNode>& rootRuleNodes) const
{
	//if (!ruleApplicable(rule)) throw "Invalid source stack";
	//if (ruleNode->symbol.isTerminal())
	//	return { shared_from_this() };
	std::list<std::shared_ptr<const State>> states;
	//if (rootRuleNode->continuations.at(ruleNode->symbol).rule != nullptr)
	//	states.push_back(shared_from_this());
	for (const std::pair<const Symbol, RuleNode>& sr : rootRuleNodes.at(ruleNode->symbol).continuations)
	{
		shared_ptr<const State> state = State::make(&sr.second, shared_from_this(), this);
		if (state != nullptr)
			states.push_back(state);
	}
	return states;
}
std::list<std::shared_ptr<const State>> State::continued(std::shared_ptr<const State> current) const
{
	std::list<std::shared_ptr<const State>> states;
	for (const std::pair<const Symbol, RuleNode>& sr : ruleNode->continuations)
	{
		shared_ptr<const State> state = State::make(&sr.second, current, parent);
		if (state != nullptr)
			states.push_back(state);
	}
	if (ruleNode->rule != nullptr && parent != nullptr)
		states.append_range(parent->continued(current));
	return states;
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
		//if (state->parent != nullptr && state->parent->ruleNode->symbol.isTerminal())
		//	continue;
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
	rule(state->rule),
	//tokenizer(state->tokenizer),
	parent(nullptr)
{}

TreeNode2<Token>::TreeNode2(const HistoryState* state):
	TreeNode1(state),
	token(state->rule->left.as<const Terminal*>()->token_type, state->tokenizer.getToken(state->rule->left.as<const Terminal*>()->token_type).value().value)
{}
void TreeNode2<Token>::out(int offset) const
{
	std::cout << std::string(offset, ' ') << rule->dbg_view << std::endl;
}
Token TreeNode2<Token>::get()
{
	return token;
}



//std::list<std::shared_ptr<State>> AnalyzerBase::expand(const std::shared_ptr<State>& leaf)
//{
//	std::list<std::shared_ptr<State>> leafs({ leaf });
//	//std::list<std::shared_ptr<State>> repeated_states;
//	for (std::list<std::shared_ptr<State>>::iterator state = std::begin(leafs); state != std::end(leafs);)
//	{
//		if (state->get()->front_symbol->isTerminal())
//		{
//			state++;
//			continue;
//		}
//		else
//		{
//			std::shared_ptr<State> current_state = *state;
//			state = leafs.erase(state);
//			for (std::pair<const Symbol, const RuleNode&> ruleNode : current_state->ruleNode->continuations)
//			{
//
//				std::shared_ptr<State> new_state = current_state->advanced(&ruleNode.second);
//				{
//					state = leafs.insert(state, new_state);
//				}
//			}
//		}
//	}
//	return leafs;
//}
AnalyzerBase::AnalyzerBase(std::list<Symbol> symbols, /*const Rule0* start_rule, std::map<const Terminal*, Rule0*> checkingRules,*/ std::list<const Rule0*> rules):
	//start_rule(start_rule),
	empty_symbol(new Terminal(Token::Type::empty)),
	end_symbol(new Terminal(Token::Type::eof))
	//checkingRules(checkingRules)
{
	for (const Rule0* rule : rules)
	{
		expandingRules[rule->left.as<const NonTerminal*>()].push_back(rule);

		RuleNode* rn = &rootRuleNodes[rule->left];
		rn->symbol = rule->left;
		for (const Symbol& s : rule->right)
		{
			rn = &rn->continuations[s];
			rn->symbol = s;
		}
		if (rule->right.empty())
		{
			rn = &rn->continuations[empty_symbol];
			rn->symbol = empty_symbol;
		}
		rn->rule = rule;
	}

	for (const Symbol& symbol : symbols)
	{
		if (symbol.is<const Terminal*>())
		{
			checkingRules[symbol.as<const Terminal*>()] = Rule2<Token>::makeChecking(symbol.as<const Terminal*>());
			RuleNode* rn = &rootRuleNodes[symbol];
			rn->symbol = symbol;
			rn = &rn->continuations[empty_symbol];
			rn->symbol = empty_symbol;
			rn->rule = checkingRules.at(symbol.as<const Terminal*>());
		}
	}
	checkingRules[end_symbol] = Rule2<Token>::makeChecking(end_symbol);
	rootRuleNodes[end_symbol].symbol = end_symbol;
	rootRuleNodes[end_symbol].continuations[empty_symbol].symbol = empty_symbol;
	rootRuleNodes[end_symbol].continuations[empty_symbol].rule = checkingRules[end_symbol];
}
std::unique_ptr<HistoryState> AnalyzerBase::analyze(const std::string& code)
{
	std::list<std::shared_ptr<const State>> leafs = { std::make_shared<State>(startRuleNode, code) };
	for (std::list<std::shared_ptr<const State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
	{
		if (leaf->get()->parent != nullptr && leaf->get()->parent->ruleNode->symbol.isTerminal() && leaf->get()->parent->ruleNode->symbol == end_symbol)
			leafs.insert(leaf, *leaf);
		//if (!leaf->get()->ruleNode->symbol.isTerminal())
		//	leafs.append_range(leaf->get()->expanded(&rootRuleNode));
		//if (leaf->get()->ruleNode->symbol.isTerminal() || rootRuleNode.continuations.at(leaf->get()->ruleNode->symbol).rule != nullptr)
		//{
		//	//if (!leaf->get()->ruleNode->symbol.isTerminal())
		//	//	leaf = leaf;
		//	leafs.append_range(leaf->get()->continued(*leaf));
		//}
		else if (leaf->get()->ruleNode->symbol.isTerminal() && leaf->get()->ruleNode->symbol == empty_symbol)
			leafs.append_range(leaf->get()->continued(*leaf));
		else
			leafs.append_range(leaf->get()->expanded(rootRuleNodes));
		if (leafs.size() == 1)
		{
			//leafs.front()->getTree()->out(0);
			std::cout << std::string_view(leaf->get()->tokenizer.code->cbegin(), leaf->get()->tokenizer.head);
			std::cout << ">>>ERROR<<<";
			std::cout << std::string_view(leaf->get()->tokenizer.head, leaf->get()->tokenizer.code->cend()) << std::endl;
			std::cout << "Excepted one of:" << std::endl;
			//for (const std::shared_ptr<const State>& expanded_leaf : expanded_leafs)
			//	std::cout << expanded_leaf->ruleNode->symbol.dbg_name << std::endl;
		}
	}
	if (leafs.size() == 0) throw "Code is invalid";
	else if (leafs.size() > 1)
	{
		for (shared_ptr<const State> s : leafs)
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