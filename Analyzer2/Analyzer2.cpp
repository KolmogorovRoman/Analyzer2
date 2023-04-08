#include <iostream>
#include <vector>
#include <list>
#include <optional>
#include <ranges>
#include <memory>
#include <string>
#include <map>

using namespace std::string_literals;

struct Symbol
{
	bool isTerminal;
	std::string name;
	Symbol(bool isTerminal, std::string name):
		isTerminal(isTerminal),
		name(name)
	{}
};

struct Rule
{
	const Symbol* left;
	std::vector<const Symbol*> right;
	std::string view;
	Rule(const Symbol* left, const std::vector<const Symbol*>& right, const std::string& view):
		left(left),
		right(right),
		view(view)
	{}
	Rule(const Symbol* left, const std::vector<const Symbol*>& right):
		left(left),
		right(right)
	{
		view = left->name + "->"s;
		if (right.size() == 0) view += "_";
		else for (const Symbol* s : right)
		{
			view += s->name;
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
	static Rule* makeStart(const Symbol* start)
	{
		return new Rule(nullptr, { start, nullptr }, start->name);
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
	static Rule* makeChecking(const Symbol* terminal)
	{
		return new Rule(terminal, {}, "["s + terminal->name + "]"s);
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

struct StackMachineState: std::enable_shared_from_this<StackMachineState>
{
	struct SymbolPointer
	{
		const StackMachineState* state;
		std::vector<const Symbol*>::const_iterator iterator;
		SymbolPointer(const StackMachineState* state, std::vector<const Symbol*>::const_iterator iterator):
			state(state),
			iterator(iterator)
		{}
		SymbolPointer(nullptr_t = nullptr):
			state(nullptr)
		{}
		const Symbol* getSymbol() const
		{
			return *iterator;
		}
		const Symbol* operator->() const
		{
			return *iterator;
		}
		const Symbol* operator*() const
		{
			return *iterator;
		}
		//SymbolPointer advanced() const
		//{
		//	const StackMachineState* state = this->state;
		//	//std::vector<const Symbol*>::const_iterator iterator = std::begin(state->rule->right);
		//	std::vector<const Symbol*>::const_iterator iterator = this->iterator;
		//	while (iterator == std::end(state->rule->right))
		//	{
		//		if (state == nullptr) throw "Stack is empty";
		//		iterator = std::next(state->parent.iterator);
		//		state = state->parent.state;
		//	}
		//	return SymbolPointer(state, iterator);
		//}
	};

	const Rule* rule;
	const std::shared_ptr<const StackMachineState> prev;
	SymbolPointer parent;
	SymbolPointer next_parent;

	StackMachineState(const Rule* rule, std::shared_ptr<const StackMachineState> prev, SymbolPointer parent):
		rule(rule),
		prev(prev),
		parent(parent),
		next_parent(this, std::begin(rule->right))
	{
		while (next_parent.iterator == std::end(next_parent.state->rule->right))
		{
			if (next_parent.state == nullptr) throw "Stack is empty";
			next_parent = SymbolPointer(next_parent.state->parent.state, std::next(next_parent.state->parent.iterator));
		}
	}
	StackMachineState(const Rule* start_rule):
		rule(start_rule),
		prev(nullptr),
		parent(nullptr),
		next_parent(this, std::begin(start_rule->right))
	{}
	std::shared_ptr<StackMachineState> advanced(const Rule* rule) const
	{
		if (rule->left != next_parent.getSymbol()) throw "Invalid source stack";
		return std::make_shared<StackMachineState>(rule, shared_from_this(), next_parent);
	}
	bool ruleApplicable(const Rule* rule) const
	{
		return rule->left == *next_parent;
	}
	std::unique_ptr<TreeNode> getTree() const
	{
		std::list<const StackMachineState*> statesList;
		const StackMachineState* state = this;
		while (state != nullptr)
		{
			statesList.push_front(state);
			state = state->prev.get();
		}

		const StackMachineState* parent = nullptr;
		//statesList.pop_front();
		std::unique_ptr<TreeNode> overRoot = std::make_unique<TreeNode>();
		TreeNode* parentNode = overRoot.get();
		for (const StackMachineState* state : statesList)
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

std::list<std::shared_ptr<StackMachineState>> expand(std::list<std::shared_ptr<StackMachineState>> leafs, std::multimap<const Symbol*, const Rule*> expandingRules)
{
	while (true)
	{
		std::shared_ptr<StackMachineState> leaf = leafs.front();
		bool leafAdvanced = false;
		auto rules_range = expandingRules.equal_range(*leaf->next_parent);
		for (std::multimap<const Symbol*, const Rule*>::const_iterator rule = rules_range.first; rule!= rules_range.second;	rule++)
		{
			if (!leaf->next_parent->isTerminal && leaf->ruleApplicable(rule->second))
			{
				leafs.push_back(leaf->advanced(rule->second));
				leafAdvanced = true;
			}
		}
		if (!leafAdvanced) break;
		leafs.pop_front();
	}
	return leafs;
	//while (true)
	//{
	//	std::list<std::shared_ptr<StackMachineState>> newLeafs;
	//	bool leafAdvanced = false;
	//	for (const std::shared_ptr<StackMachineState>& l : leafs)
	//	{
	//		const Symbol* leafStackTop = *l->next_parent;
	//		if (!leafStackTop->isTerminal)
	//		{
	//			for (const Rule* r : expandingRules)
	//			{
	//				if (l->ruleApplicable(r))
	//				{
	//					leafAdvanced = true;
	//					newLeafs.push_back(l->advanced(r));
	//				}
	//			}
	//		}
	//	}
	//	if (!leafAdvanced) break;
	//	leafs = newLeafs;
	//}
	//return leafs;
}

int main()
{
	/*Symbol* variable = new Symbol(true, "variable");
	Symbol* oper = new Symbol(true, "oper");
	Symbol* expression = new Symbol(false, "expression");

	std::vector<Symbol*> symbols{ variable, oper, expression };

	std::vector<Rule> rules;
	rules.push_back(Rule(expression, std::vector<Symbol*>{variable}));
	rules.push_back(Rule(expression, std::vector<Symbol*>{expression, oper, variable}));

	std::vector<Rule> rules;
	for (const Rule& r : rules)
	{
		Symbol* left = r.left;
		std::vector<Symbol*> right = r.right;
		rules.push_back(Rule(left, right));
	}
	for (Symbol* s : symbols)
	{
		Symbol* input = s;
		Symbol* left = s;
		std::vector<Symbol*> right = {};
		rules.push_back(Rule(input, left, right));
	}

	StackMachineState s(nullptr, nullptr, );
	std::cout << "Hello World!\n";*/
	Symbol* a = new Symbol(true, "a");
	Symbol* b = new Symbol(true, "b");
	Symbol* c = new Symbol(true, "c");
	Symbol* A = new Symbol(false, "A");
	Symbol* B = new Symbol(false, "B");
	Symbol* C = new Symbol(false, "C");

	const Rule* start_rule = Rule::makeStart(A);
	std::multimap<const Symbol*, const Rule*> expandingRules
	{
		{A, new Rule(A, { B, C })},
		{B, new Rule(B, { })},
		{B, new Rule(B, { b, B })},
		{C, new Rule(C, { c })}
	};
	std::map<const Symbol*, Rule*> checkingRules;
	checkingRules[b] = Rule::makeChecking(b);
	checkingRules[c] = Rule::makeChecking(c);

	std::list<const Symbol*> input{ b, b, c };

	std::shared_ptr<StackMachineState> start = std::make_shared<StackMachineState>(start_rule);
	std::list<std::shared_ptr<StackMachineState>> leafs = { start };
	for (const Symbol* i : input)
	{
		leafs = expand(leafs, expandingRules);
		std::list<std::shared_ptr<StackMachineState>> newLeafs;
		for (const std::shared_ptr<StackMachineState>& l : leafs)
		{
			const Symbol* leafStackTop = *l->next_parent;
			if (!leafStackTop->isTerminal)
			{
				throw "There must be terminals only";
			}
			if (leafStackTop == i)
			{
				Rule* c = checkingRules[leafStackTop];
				newLeafs.push_back(l->advanced(c));
			}
		}
		leafs = newLeafs;
	}
	//std::cout << std::endl;
	std::unique_ptr<TreeNode> root = leafs.back()->getTree();
	root->out(0);
}