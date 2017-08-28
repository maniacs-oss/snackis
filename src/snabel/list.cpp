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
    if (in->empty()) { return nullopt; }
    auto res(in->front());
    in->pop_front();
    return res;
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

  static void list_zero_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.bool_type, get<ListRef>(in)->empty());
  }

  static void list_pos_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.bool_type, !get<ListRef>(in)->empty());
  }
  
  static void list_push_imp(Scope &scp, const Args &args) {
    auto &lst(args.at(0));
    auto &el(args.at(1));
    get<ListRef>(lst)->push_back(el);
    push(scp.thread, lst);    
  }

  static void list_pop_imp(Scope &scp, const Args &args) {
    auto &lst_arg(args.at(0));
    auto &lst(*get<ListRef>(lst_arg));
    push(scp.thread, lst_arg);
    push(scp.thread, lst.back());
    lst.pop_back();
  }

  static void list_reverse_imp(Scope &scp, const Args &args) {
    auto &in_arg(args.at(0));
    auto &in(*get<ListRef>(in_arg));
    std::reverse(in.begin(), in.end());
    push(scp.thread, in_arg); 
  }

  static void list_sort_imp(Scope &scp, const Args &args) {
    auto &thd(scp.thread);
    auto &in_arg(args.at(0));
    auto &in(*get<ListRef>(in_arg));
    auto &tgt(args.at(1));
    push(scp.thread, in_arg); 

    std::stable_sort(in.begin(), in.end(), [&scp, &thd, &tgt](auto &x, auto &y) {
	push(thd, x);
	push(thd, y);
	(*tgt.type->call)(scp, tgt, true);
	auto res(try_pop(thd));

	if (!res) {
	  ERROR(Snabel, "Missing comparison result while sorting");
	  return false;
	}

	return get<bool>(*res);
      });    
  }

  static void list_unzip_imp(Scope &scp, const Args &args) {
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

  static void list_fifo_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    auto &elt(*in.type->args.at(0));
    
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

    add_func(exe, "list",
	     {ArgType(exe.iterable_type)},
	     {ArgType([&exe](auto &args) {
		   return &get_list_type(exe,
					 *get_sub(exe,
						  *args.at(0).type,
						  exe.iterable_type)->args.at(0));
		 })},
	     iterable_list_imp);

    add_func(exe, "nlist",
	     {ArgType(exe.iterable_type), ArgType(exe.i64_type)},
	     {ArgType([&exe](auto &args) {
		   return &get_list_type(exe, *args.at(0).type->args.at(0));
		 })},
	     iterable_nlist_imp);

    add_func(exe, "list",
	     {ArgType(exe.meta_type)},
	     {ArgType([&exe](auto &args) {
		   return &get_list_type(exe, *args.at(0).type);
		 })},
	     list_imp);

    add_func(exe, "z?",
	     {ArgType(exe.list_type)}, {ArgType(exe.bool_type)},
	     list_zero_imp);
    
    add_func(exe, "+?",
	     {ArgType(exe.list_type)}, {ArgType(exe.bool_type)},
	     list_pos_imp);
    
    add_func(exe, "push",
	     {ArgType(exe.list_type), ArgType(0, 0)}, {ArgType(0)},
	     list_push_imp);
    
    add_func(exe, "pop",
	     {ArgType(exe.list_type)}, {ArgType(0), ArgType(0, 0)},
	     list_pop_imp);

    add_func(exe, "reverse",
	     {ArgType(exe.list_type)}, {ArgType(0)},
	     list_reverse_imp);

    add_func(exe, "sort",
	     {ArgType(exe.list_type), ArgType(exe.callable_type)},
	     {ArgType(0)},
	     list_sort_imp);

    add_func(exe, "unzip",
	     {ArgType(get_list_type(exe, exe.pair_type))},
	     {ArgType([&exe](auto &args) {
		   return &get_iter_type(exe,
					 *args.at(0).type->args.at(0)->args.at(0));
		 }),
		 ArgType([&exe](auto &args) {
		     return &get_iter_type(exe,
					   *args.at(0).type->args.at(0)->args.at(1));
		   })},			
	     list_unzip_imp);

    add_func(exe, "fifo",
	     {ArgType(exe.list_type)},
	     {ArgType([&exe](auto &args) {
		   return &get_iter_type(exe, *args.at(0).type->args.at(0));
		 })},
	     list_fifo_imp);
  }
  
  Type &get_list_type(Exec &exe, Type &elt) {    
    str n(fmt("List<%0>", elt.name));
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
