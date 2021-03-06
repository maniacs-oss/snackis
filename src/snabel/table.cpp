#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/pair.hpp"
#include "snabel/table.hpp"

namespace snabel {
  TableIter::TableIter(Exec &exe, Type &elt, const TableRef &in):
    Iter(exe, get_iter_type(exe, elt)), elt(elt), in(in), it(in->begin())
  { }
  
  opt<Box> TableIter::next(Scope &scp) {
    if (it == in->end()) { return nullopt; }
    auto res(*it);
    it++;
    return Box(scp, elt, std::make_pair(res.first, res.second));
  }

  static void table_imp(Scope &scp, const Args &args) {
    auto &key(args.at(0));
    auto &val(args.at(1));
    
    push(scp,
	 get_table_type(scp.exec, *get<Type *>(key), *get<Type *>(val)),
	 std::make_shared<Table>());    
  }

  static void iter_table_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    auto out(std::make_shared<Table>());
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));

    while (true) {
      auto nxt(it->next(scp));
      if (!nxt) { break; }
      out->insert(get<Pair>(*nxt));
    }
    
    push(scp,
	 get_table_type(exe, *elt.args.at(0), *elt.args.at(1)),
	 out);
  }

  static void len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.i64_type, (int64_t)get<TableRef>(in)->size());
  } 

  static void zero_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.bool_type, get<TableRef>(in)->empty());
  }

  static void pos_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.bool_type, !get<TableRef>(in)->empty());
  }
  
  static void get_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &tbl_arg(args.at(0));
    auto &tbl(*get<TableRef>(tbl_arg));
    auto &key(args.at(1));
    auto fnd(tbl.find(key));
    
    if (fnd == tbl.end()) {
      push(scp, *tbl_arg.type->args.at(1));
    } else {
      push(scp, get_opt_type(exe, *fnd->second.type), fnd->second.val);
    }
  }

  static void put_imp(Scope &scp, const Args &args) {
    auto &tbl(*get<TableRef>(args.at(0)));
    auto &key(args.at(1));
    auto &val(args.at(2));
    auto fnd(tbl.find(key));
    
    if (fnd == tbl.end()) {
      tbl.emplace(std::piecewise_construct,
		  std::forward_as_tuple(key),
		  std::forward_as_tuple(val));
    } else {
      fnd->second = val;
    }
  }

  static void upsert_imp(Scope &scp, const Args &args) {
    auto &tbl(*get<TableRef>(args.at(0)));
    auto &key(args.at(1));
    auto &val(args.at(2));
    auto &tgt(args.at(3));    
    auto fnd(tbl.find(key));
    
    if (fnd == tbl.end()) {
      tbl.emplace(std::piecewise_construct,
		  std::forward_as_tuple(key),
		  std::forward_as_tuple(val));
    } else {
      push(scp.thread, fnd->second);
      tgt.type->call(scp, tgt, true);
      auto res(try_pop(scp.thread));

      if (res) {
	fnd->second = *res;
      } else {
	ERROR(Snabel, "Missing table upsert value");
      }
    }
  }

  static void del_imp(Scope &scp, const Args &args) {
    auto &tbl(*get<TableRef>(args.at(0)));
    auto &key(args.at(1));
    tbl.erase(key);
  }

  void init_tables(Exec &exe) {
    exe.table_type.supers.push_back(&exe.any_type);
    exe.table_type.args.push_back(&exe.any_type);

    exe.table_type.uneval = [](auto &v, auto &out) {
      uneval(*get<TableRef>(v), out);
    };
    
    exe.table_type.dump = [](auto &v) { return dump(*get<TableRef>(v)); };
    exe.table_type.fmt = [](auto &v) { return table_fmt(*get<TableRef>(v)); };

    exe.table_type.eq = [](auto &x, auto &y) {
      return get<TableRef>(x) == get<TableRef>(y);
    };

    exe.table_type.iter = [&exe](auto &in) {
      return IterRef(new TableIter(exe,
				   get_pair_type(exe,
						 *in.type->args.at(0),
						 *in.type->args.at(1)),
				   get<TableRef>(in)));
    };

    exe.table_type.equal = [](auto &x, auto &y) {
      auto &xs(*get<TableRef>(x)), &ys(*get<TableRef>(y));
      auto xi(xs.begin()), yi(ys.begin());

      for (; xi != xs.end() && yi != ys.end(); xi++, yi++) {
	if (xi->first.type != yi->first.type ||
	    xi->second.type != yi->second.type ||
	    !xi->first.type->equal(xi->first, yi->first) ||
	    !xi->second.type->equal(xi->second, yi->second)) { return false; }
      }

      return true;
    };

    add_func(exe, "table", Func::Const,
	     {ArgType(exe.meta_type), ArgType(exe.meta_type)},
	     table_imp);
    
    add_func(exe, "table", Func::Safe,
	     {ArgType(get_iterable_type(exe, exe.pair_type))},
	     iter_table_imp);

    add_func(exe, "len", Func::Const,
	     {ArgType(exe.table_type)},
	     len_imp);

    add_func(exe, "z?", Func::Const,
	     {ArgType(exe.table_type)},
	     zero_imp);
    
    add_func(exe, "+?", Func::Const,
	     {ArgType(exe.table_type)},
	     pos_imp);
    
    add_func(exe, "get", Func::Const,
	     {ArgType(exe.table_type), ArgType(0, 0)},
	     get_imp);

    add_func(exe, "put", Func::Safe,
	     {ArgType(exe.table_type), ArgType(0, 0), ArgType(0, 1)},
	     put_imp);

    add_func(exe, "del", Func::Safe,
	     {ArgType(exe.table_type), ArgType(0, 0)},
	     del_imp);

    add_func(exe, "upsert", Func::Safe,
	     {ArgType(exe.table_type), ArgType(0, 0), ArgType(0, 1),
		 ArgType(exe.callable_type)},
	     upsert_imp);
  }

  Type &get_table_type(Exec &exe, Type &key, Type &val) {    
    auto &n(get_sym(exe, fmt("Table<%0 %1>", name(key.name), name(val.name))));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.table_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&get_iterable_type(exe, get_pair_type(exe, key, val)));
    t.supers.push_back(&exe.table_type);
    t.args.push_back(&key);
    t.args.push_back(&val);
    t.dump = exe.table_type.dump;
    t.fmt = exe.table_type.fmt;
    t.eq = exe.table_type.eq;
    t.equal = exe.table_type.equal;
    t.iter = exe.table_type.iter;
    return t;
  }

  void uneval(const Table &tbl, std::ostream &out) {
    out << '[';
    size_t i(0);
    
    for (auto it(tbl.begin()); it != tbl.end(); it++, i++) {
      if (i > 0) { out << ' '; }
      uneval(*it, out);
    }
    
    out << ']';
  }

  str dump(const Table &tbl) {
    OutStream buf;
    buf << '[';
    size_t i(0);
    
    for (auto it(tbl.begin()); it != tbl.end(); it++, i++) {
      if (i > 0) { buf << ' '; }
      buf << dump(*it);
      
      if (i == 100) {
	buf << "..." << tbl.size();
	break;
      }
    }
    
    buf << ']';
    return buf.str();
  }

  str table_fmt(const Table &tbl) {
    OutStream buf;
    buf << '[';
    size_t i(0);
    
    for (auto it(tbl.begin()); it != tbl.end(); it++, i++) {
      if (i > 0) { buf << ' '; }
      buf << pair_fmt(*it);
      
      if (i == 100) {
	buf << "..." << tbl.size();
	break;
      }
    }
    
    buf << ']';
    return buf.str();
  }
  
  bool nil(const Box &b) {
    return !b.val.has_value();
  }
}
