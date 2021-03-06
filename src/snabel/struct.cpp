#include <iostream>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/pair.hpp"
#include "snabel/struct.hpp"

namespace snabel {
  Struct::Struct(Type &t):
    type(t)
  { }

  StructIter::StructIter(Exec &exe, const StructRef &in):
    Iter(exe, get_iter_type(exe, get_pair_type(exe, exe.sym_type, exe.any_type))),
    in(in), it(in->fields.begin())
  { }
  
  opt<Box> StructIter::next(Scope &scp) {
    if (it == in->fields.end()) { return nullopt; }
    auto res(*it);
    it++;

    return Box(scp,
	       get_pair_type(exec, exec.sym_type, *res.second.type), 
	       std::make_pair(Box(scp, exec.sym_type, res.first),
			      res.second));
  }

  bool operator ==(const Struct &x, const Struct &y) {
    for (auto xi(x.fields.begin()), yi(y.fields.begin());
	 xi != x.fields.end() && yi != y.fields.end();
	 xi++, yi++) {
      if (*xi == *yi) { continue; }
      return false;
    }

    return true;
  }
  
  static void new_imp(Scope &scp, const Args &args) {
    auto &t(*get<Type *>(args.at(0)));
    push(scp, t, std::make_shared<Struct>(t));
  }
  
  void init_structs(Exec &exe) {
    exe.struct_type.supers.push_back(&exe.any_type);
    auto &it_type(get_iterable_type(exe,
				    get_pair_type(exe,
						 exe.sym_type,
						 exe.any_type)));
    exe.struct_type.supers.push_back(&it_type);
    exe.struct_type.fmt = [](auto &v) { return fmt_arg(*get<StructRef>(v)); };
    
    exe.struct_type.eq = [](auto &x, auto &y) {
      return get<StructRef>(x) == get<StructRef>(y);
    };

    exe.struct_type.equal = [](auto &x, auto &y) {
      return *get<StructRef>(x) == *get<StructRef>(y);
    };

    exe.struct_type.iter = [&exe](auto &in) {
      return IterRef(new StructIter(exe, get<StructRef>(in)));
    };
    
    auto &meta(get_meta_type(exe, exe.struct_type));
    add_func(exe, "new", Func::Const, {ArgType(meta)}, new_imp);
    
    add_macro(exe, "struct:", [&exe](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed struct definition on row %0, col %1",
			    pos.row, pos.col));
	  return;
	}

	auto &n(in.at(0).text);
	  
	if (!isupper(n.at(0))) {
	  ERROR(Snabel, fmt("Struct name should be capitalized: %0", n));
	  return;
	}
	  
	auto &ns(get_sym(exe, n));
	auto &t(add_struct_type(exe, ns));

	in.pop_front();
	auto end(find_end(in.begin(), in.end()));
	auto &thd(curr_thread(exe));
	  
	while (in.begin() != end) {
	  auto tn(in.front().text);
	  if (!isupper(tn.at(0))) { break; }
	  in.pop_front();	    

	  auto st(parse_type(exe, tn, 0).first);
	  if (!st) { break; }
	    
	  if (!isa(thd, *st, exe.struct_type)) {
	    ERROR(Snabel, fmt("Invalid super struct: %0", name(st->name)));
	    break;
	  }
	    
	  t.supers.push_back(st);
	  continue;
	}

	std::vector<str> field_names;

	auto add_fields([&](auto &ft) {
	    for (auto &fn: field_names) {
	      auto fns(get_sym(exe, fn));
		
	      add_func(exe, fns, Func::Const,
		       {ArgType(t)},
		       [n, fn, fns, &ft](auto &scp, auto &args) {
			 auto &thd(scp.thread);
			 auto &self(args.at(0));
			 auto &fs(get<StructRef>(self)->fields);
			 auto fnd(fs.find(fns));
		       
			 if (fnd == fs.end()) {
			   ERROR(Snabel, 
				 snackis::fmt("Reading uninitialized field: %0.%1",
					      n, fn));
			 } else {
			   push(thd, fnd->second);
			 }
		       });
	    
	      add_func(exe, snackis::fmt("set-%0", fn), Func::Safe,
		       {ArgType(t), ArgType(ft)},
		       [fns](auto &scp, auto &args) {
			 auto &self(args.at(0));
			 auto &fs(get<StructRef>(self)->fields);
			 auto &v(args.at(1));
			 auto res(fs.emplace(std::piecewise_construct,
					     std::forward_as_tuple(fns),
					     std::forward_as_tuple(v)));
			 if (!res.second) { res.first->second = v; }
		       });		
	    }
	  });
	  
	while (in.begin() != end) {	           
	  str &tok(in.front().text);
	    
	  if (isupper(tok.at(0))) {
	    auto ft(parse_type(exe, tok, 0).first);
	    if (!ft) { break; }
	    add_fields(*ft);
	  } else {
	    field_names.push_back(tok);
	  }

	  in.pop_front();
	}

	if (!field_names.empty()) { add_fields(exe.any_type); }

	if (in.at(0).text != ";") {
	  ERROR(Snabel, fmt("Malformed struct definition on row %0, col %1",
			    pos.row, pos.col));
	  return;
	}
	
	in.pop_front();
      });
  }

  Type &add_struct_type(Exec &exe, const Sym &n) {
    auto &t(add_type(exe, n));
    t.supers.push_back(&exe.struct_type);
    t.fmt = exe.struct_type.fmt;
    t.eq = exe.struct_type.eq;
    t.equal = exe.struct_type.equal;
    t.iter = exe.struct_type.iter;
    return t;
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Struct &stc) {
    OutStream out;
    out << name(stc.type.name) << '(';
    str sep("");

    for (auto [k, v]: stc.fields) {
      out << sep << name(k) << ' ' << fmt_arg(v) << '.';
      sep = " ";
    }
    
    out << ')';
    return out.str();
  }
}

