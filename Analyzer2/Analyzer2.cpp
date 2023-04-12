#include <iostream>
#include <vector>
#include <list>
#include <optional>
#include <ranges>
#include <memory>
#include <string>
#include <map>
#include <ranges>

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
	static Rule* makeStart(const Symbol* start, const Symbol* end)
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

struct State: std::enable_shared_from_this<State>
{
	struct SymbolPointer
	{
		const State* state;
		std::vector<const Symbol*>::const_iterator iterator;
		SymbolPointer(const State* state, std::vector<const Symbol*>::const_iterator iterator):
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
	};

	const Rule* rule;
	const std::shared_ptr<const State> prev;
	SymbolPointer parent;
	SymbolPointer next_parent;

	State(const Rule* rule, std::shared_ptr<const State> prev, SymbolPointer parent):
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
	State(const Rule* start_rule):
		rule(start_rule),
		prev(nullptr),
		parent(nullptr),
		next_parent(this, std::begin(start_rule->right))
	{}
	std::shared_ptr<State> advanced(const Rule* rule) const
	{
		if (rule->left != *next_parent) throw "Invalid source stack";
		return std::make_shared<State>(rule, shared_from_this(), next_parent);
	}
	bool ruleApplicable(const Rule* rule) const
	{
		return rule->left == *next_parent;
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
	std::map<std::string, const Symbol*> symbolbyName;

	const Rule* start_rule;
	const Symbol* end_symbol;
	std::map<const Symbol*, std::list<const Rule*>> expandingRules;
	std::map<const Symbol*, Rule*> checkingRules;
	std::list<std::shared_ptr<State>> expand(const std::shared_ptr<State>& leaf)
	{
		std::list<std::shared_ptr<State>> leafs({ leaf });
		std::list<std::shared_ptr<State>>::iterator state = std::begin(leafs);
		while (state != std::end(leafs))
		{
			std::shared_ptr<State> leaf = *state;
			if (leaf->next_parent->isTerminal)
				state++;
			else
			{
				std::ranges::view auto leafs_to_insert =
					std::views::transform(expandingRules[*leaf->next_parent],
										  [&](const Rule* rule) { return leaf->advanced(rule); });
				state = leafs.erase(state);
				state = leafs.insert(state, leafs_to_insert.begin(), leafs_to_insert.end());
			}
		}
		return leafs;
	}
public:
	Analyzer(std::list<const Symbol*> symbols, const Symbol* start, std::list<const Rule*> rules)
	{
		for (const Symbol* symbol : symbols)
		{
			if (symbol->isTerminal)
				checkingRules[symbol] = Rule::makeChecking(symbol);
		}
		end_symbol = new Symbol(true, "eof");
		checkingRules[end_symbol] = Rule::makeChecking(end_symbol);
		start_rule = Rule::makeStart(start, end_symbol);
		for (const Rule* rule : rules)
		{
			expandingRules[rule->left].push_back(rule);
		}
	}
	Analyzer(const std::list<std::string>& terminals,
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
	}
	std::unique_ptr<TreeNode> analyze(std::list<const Symbol*> input)
	{
		input.push_back(end_symbol);
		std::list<std::shared_ptr<State>> leafs = { std::make_shared<State>(start_rule) };
		for (const Symbol* i : input)
			for (std::list<std::shared_ptr<State>>::iterator leaf = std::begin(leafs); leaf != std::end(leafs); leaf = leafs.erase(leaf))
				for (const std::shared_ptr<State>& expanded_leaf : expand(*leaf))
					if (expanded_leaf->ruleApplicable(checkingRules[i]))
						leafs.insert(leaf, expanded_leaf->advanced(checkingRules[*expanded_leaf->next_parent]));

		return leafs.back()->getTree();
	}
	std::unique_ptr<TreeNode> analyze(std::list<std::string> text_input)
	{
		std::list<const Symbol*> input;
		for (const std::string& i : text_input)
			input.push_back(symbolbyName[i]);
		return analyze(input);
	}
};

int main()
{
	std::unique_ptr<TreeNode> root = Analyzer(
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
	root->out(0);
}