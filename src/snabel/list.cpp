#include <iostream>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/list.hpp"

namespace snabel {
  ListIter::ListIter(Exec &exe, Type &elt, const ListRef &in, opt<Fn> fn):
    Iter(exe, get_iter_type(exe, elt)), elt(elt), in(in), it(in->begin()), fn(fn)
  { }
  
  opt<Box> ListIter::next(Scope &scp) {
    if (it == in->end()) { return nullopt; }
    auto res(*it);
    it++;
    return fn ? (*fn)(res) : res;
  }

  FifoIter::FifoIter(Exec &exe, Type &elt, const ListRef &in):
    Iter(exe, get_iter_type(exe, elt)), in(in)
  { }
  
  opt<Box> FifoIter::next(Scope &scp) {
    if (in->empty()) { return Box(*type.args.at(0), nil); }
    auto res(in->front());
    in->pop_front();
    return Box(*type.args.at(0), res.val);
  }

  static void iterable_list_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto out(std::make_shared<List>());
    Type *elt(nullptr);
    
    while (true) {
      auto v(it->next(scp));
      if (!v) { break; }
      out->push_back(*v);
      elt = elt ? get_super(scp.exec, *elt, *v->type) : v->type;
    }
    
    push(scp.thread, get_list_type(scp.exec, *elt), out); 
  }

  static void iterable_nlist_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto out(std::make_shared<List>());
    
    for (int64_t i(0); i < get<int64_t>(args.at(1)); i++) {
      auto v(it->next(scp));
      if (!v) { break; }
      out->push_back(*v);
    }
    
    push(scp.thread, get_list_type(scp.exec, *it->type.args.at(0)), out); 
  }

  static void list_imp(Scope &scp, const Args &args) {
    auto &elt(args.at(0));
    push(scp.thread,
	 get_list_type(scp.exec, *get<Type *>(elt)),
	 std::make_shared<List>());    
  }

  static void zero_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, scp.exec.bool_type, get<ListRef>(in)->empty());
  }

  static void pos_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, scp.exec.bool_type, !get<ListRef>(in)->empty());
  }

  static void len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, scp.exec.i64_type, (int64_t)get<ListRef>(in)->size());
  }

  static void push_imp(Scope &scp, const Args &args) {
    auto &lst(args.at(0));
    auto &el(args.at(1));
    get<ListRef>(lst)->push_back(el);
  }

  static void pop_imp(Scope &scp, const Args &args) {
    auto &lst(*get<ListRef>(args.at(0)));
    push(scp.thread, lst.back());
    lst.pop_back();
  }

  static void reverse_imp(Scope &scp, const Args &args) {
    auto &in(*get<ListRef>(args.at(0)));
    std::reverse(in.begin(), in.end());
  }

  static void clear_imp(Scope &scp, const Args &args) {
    get<ListRef>(args.at(0))->clear();
  }

  static void sort_imp(Scope &scp, const Args &args) {
    auto &thd(scp.thread);
    auto &in(*get<ListRef>(args.at(0)));
    auto &tgt(args.at(1));

    std::stable_sort(in.begin(), in.end(), [&scp, &thd, &tgt](auto &x, auto &y) {
	push(thd, x);
	push(thd, y);
	tgt.type->call(scp, tgt, true);
	auto res(try_pop(thd));

	if (!res) {
	  ERROR(Snabel, "Missing comparison result while sorting");
	  return false;
	}

	return get<bool>(*res);
      });    
  }

  static void unzip_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));

    auto &yt(*in.type->args.at(0)->args.at(0));
    push(scp.thread,
	 get_iter_type(exe, yt),
	 IterRef(new ListIter(exe, yt, get<ListRef>(in), [](auto &el) {
	       return get<PairRef>(el)->first;
	     })));

    auto &xt(*in.type->args.at(0)->args.at(1));
    push(scp.thread,
	 get_iter_type(exe, xt),
	 IterRef(new ListIter(exe, xt, get<ListRef>(in), [](auto &el) {
	       return get<PairRef>(el)->second;
	     })));
  }

  static void fifo_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    auto &elt(get_opt_type(exe, *in.type->args.at(0)));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new FifoIter(exe, elt, get<ListRef>(in))));
  }
  
  void init_lists(Exec &exe) {
    exe.list_type.supers.push_back(&exe.any_type);
    exe.list_type.args.push_back(&exe.any_type);
    exe.list_type.dump = [](auto &v) { return dump(*get<ListRef>(v)); };
    exe.list_type.fmt = [](auto &v) { return list_fmt(*get<ListRef>(v)); };

    exe.list_type.eq = [](auto &x, auto &y) {
      return get<ListRef>(x) == get<ListRef>(y);
    };

    exe.list_type.iter = [&exe](auto &in) {
      return IterRef(new ListIter(exe, *in.type->args.at(0), get<ListRef>(in)));
    };

    exe.list_type.equal = [](auto &x, auto &y) {
      auto &xs(*get<ListRef>(x)), &ys(*get<ListRef>(y));
      if (xs.size() != ys.size()) { return false; }
      auto xi(xs.begin()), yi(ys.begin());

      for (; xi != xs.end() && yi != ys.end(); xi++, yi++) {
	if (xi->type != yi->type || !xi->type->equal(*xi, *yi)) { return false; }
      }

      return true;
    };

    add_func(exe, "list", {ArgType(exe.iterable_type)}, iterable_list_imp);

    add_func(exe, "nlist",
	     {ArgType(exe.iterable_type), ArgType(exe.i64_type)},
	     iterable_nlist_imp);

    add_func(exe, "list", {ArgType(exe.meta_type)}, list_imp);
    add_func(exe, "z?", {ArgType(exe.list_type)}, zero_imp);
    add_func(exe, "+?", {ArgType(exe.list_type)}, pos_imp);
    add_func(exe, "len", {ArgType(exe.list_type)}, len_imp);

    add_func(exe, "push",
	     {ArgType(exe.list_type), ArgType(0, 0)},
	     push_imp);
    
    add_func(exe, "pop", {ArgType(exe.list_type)}, pop_imp);
    add_func(exe, "reverse", {ArgType(exe.list_type)}, reverse_imp);

    add_func(exe, "sort",
	     {ArgType(exe.list_type), ArgType(exe.callable_type)},
	     sort_imp);

    add_func(exe, "clear", {ArgType(exe.list_type)}, clear_imp);

    add_func(exe, "unzip",
	     {ArgType(get_list_type(exe, exe.pair_type))},
	     unzip_imp);

    add_func(exe, "fifo", {ArgType(exe.list_type)}, fifo_imp);
  }
  
  Type &get_list_type(Exec &exe, Type &elt) {    
    auto &n(get_sym(exe, fmt("List<%0>", name(elt.name))));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.list_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&get_iterable_type(exe, elt));
    t.supers.push_back(&exe.list_type);
    t.args.push_back(&elt);
    t.dump = exe.list_type.dump;
    t.fmt = exe.list_type.fmt;
    t.eq = exe.list_type.eq;
    t.equal = exe.list_type.equal;
    t.iter = exe.list_type.iter;
    return t;
  }
}
