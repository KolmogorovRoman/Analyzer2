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

template<class T>
Rule1<T>::Rule1(Symbol left, const std::vector<Symbol>& right, const std::string& dbg_view):
	Rule0(left, right, dbg_view)
{}
template<class T>
Rule1<T>::Rule1(const NonTerminal* left, const std::vector<Symbol>& right) :
	Rule0(left, right)
{}

template <class T, class... Childs>
Rule2<T, Childs...>::Rule2(Symbol left, const std::vector<Symbol>& right, const std::string& view) :
	Rule1<T>(left, right, view)
{}
template <class T, class... Childs>
Rule2<T, Childs...>::Rule2(const NonTerminal* left, const std::vector<Symbol>& right, std::function<T(Childs...)> get) :
	Rule1<T>(left, right),
	get(get)
{}
template<class T, class... Childs>
TreeNode1<T>* Rule2<T, Childs...>::make_tree_node(const HistoryState* state) const
{
	return new TreeNode2<T, Childs...>(state, std::index_sequence_for<Childs...>());
}

template<class T>
TreeNode1<T>::TreeNode1(const HistoryState* state):
	TreeNode0(state)
{}

template<class T>
T ChildFiller<T>::get(stored_type node)
{
	return node->get();
}
template<class T>
TreeNode1<T>* ChildFiller<TreeNode1<T>*>::get(stored_type node)
{
	return node;
}

template<class T, class... Childs>
template<size_t... inds>
TreeNode2<T, Childs...>::TreeNode2(const HistoryState* state, std::index_sequence<inds...>):
	TreeNode1<T>(state),
	childs(static_cast<const Rule1<typename ChildFiller<Childs>::rule_type>*>(state->childs[inds]->state->rule)->make_tree_node(state->childs[inds].get())...)
{}
template<class T, class... Childs>
template <std::size_t... Is>
void TreeNode2<T, Childs...>::out_childs(int offset, std::index_sequence<Is...>) const
{
	(std::get<Is>(childs)->out(offset), ...);
}
template<class T, class... Childs>
void TreeNode2<T, Childs...>::out(int offset) const
{
	std::cout << std::string(offset, ' ') << TreeNode0::rule->dbg_view << std::endl;
	out_childs(offset + 1, std::index_sequence_for<Childs...>());
}

template<class T, class... Childs>
template <std::size_t... Is>
std::tuple<Childs...> TreeNode2<T, Childs...>::fill_childs(std::index_sequence<Is...>)
{
	return std::tuple(ChildFiller<Childs>::get(std::get<Is>(childs))...);
}
template<class T, class... Childs>
T TreeNode2<T, Childs...>::get()
{
	std::tuple<Childs...> tuple_childs = fill_childs(std::index_sequence_for<Childs...>());
	return std::apply(static_cast<const Rule2<T, Childs...>*>(TreeNode0::rule)->get, tuple_childs);
}

