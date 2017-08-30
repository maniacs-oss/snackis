#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/struct.hpp"

namespace snabel {
  Struct::Struct(Type &t):
    type(t)
  { }

  bool operator==(const Struct &x, const Struct &y) {
    if (x.fields.size() != y.fields.size()) { return false; }
    
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
    push(scp.thread, t, std::make_shared<Struct>(t));
  }
  
  void init_structs(Exec &exe) {
    exe.struct_type.supers.push_back(&exe.any_type);
    exe.struct_type.fmt = [](auto &v) { return fmt_arg(*get<StructRef>(v)); };
    
    exe.struct_type.eq = [](auto &x, auto &y) {
      return get<StructRef>(x) == get<StructRef>(y);
    };

    exe.struct_type.equal = [](auto &x, auto &y) {
      return *get<StructRef>(x) == *get<StructRef>(y);
    };

    auto &meta(get_meta_type(exe, exe.struct_type));
    add_func(exe, "new", {ArgType(meta)}, new_imp);
    
    add_macro(exe, "struct:", [&exe](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed struct on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup(false));
	  auto &n(get_sym(exe, in.at(0).text));

	  if (find_type(exe, n)) {
	    ERROR(Snabel, fmt("Redefining struct: %0", name(n)));
	  }
	  
	  auto &t(get_struct_type(exe, n));

	  in.pop_front();
	  auto end(find_end(in.begin(), in.end()));

	  while (in.begin() != end) {
	    auto fn(in.front().text);
	    auto &fns(get_sym(exe, in.front().text));
	    in.pop_front();

	    if (in.begin() == end) {
	      ERROR(Snabel, fmt("Malformed struct on row %0, col %1",
				pos.row, pos.col));
	      break;
	    }
       
	    str ftn(in.front().text);
	    in.pop_front();
	    auto ft(parse_type(exe, ftn, 0).first);

	    if (!ft) {
	      ERROR(Snabel, fmt("Missing field type: %0", ftn));
	      break;
	    }

	    add_func(exe, fns, {ArgType(t)}, [&n, &fns, ft](auto &scp, auto &args) {
		auto &thd(scp.thread);
		auto &fs(get<StructRef>(args.at(0))->fields);
		auto fnd(fs.find(fns));
		
		if (fnd == fs.end()) {
		  ERROR(Snabel, fmt("Reading uninitialized field: %0.%1",
				    name(n), name(fns)));
		  
		  push(thd, get_opt_type(scp.exec, *ft), nil);
		} else {
		  push(thd, fnd->second);
		}
	      });
	    
	    add_func(exe, fmt("set-%0", fn),
		     {ArgType(t), ArgType(*ft)},
		     [&n, &fns, &ft](auto &scp, auto &args) {
		       auto &sa(args.at(0));
		       auto &fs(get<StructRef>(sa)->fields);
		       auto &v(args.at(1));
		       auto res(fs.emplace(std::piecewise_construct,
					   std::forward_as_tuple(fns),
					   std::forward_as_tuple(v)));
		       if (!res.second) { res.first->second = v; }
		       push(scp.thread, sa);
		     });
	  }
	    
	  if (end != in.end()) { in.pop_front(); }
	}
      });
  }

  Type &get_struct_type(Exec &exe, const Sym &n) {
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.struct_type;
    t.supers.push_back(&exe.struct_type);
    t.fmt = exe.struct_type.fmt;
    t.eq = exe.struct_type.eq;
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

