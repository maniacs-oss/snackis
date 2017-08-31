#include "snabel/exec.hpp"
#include "snabel/iters.hpp"
#include "snabel/str.hpp"

namespace snabel {
  StrIter::StrIter(Exec &exe, const StrRef &in):
    Iter(exe, get_iter_type(exe, exe.char_type)), in(in), i(in->begin())
  { }

  opt<Box> StrIter::next(Scope &scp){
    if (i == in->end()) { return nullopt; }
    auto res(*i);
    i++;
    return Box(exec.char_type, res);
  }

  UStrIter::UStrIter(Exec &exe, const UStrRef &in):
    Iter(exe, get_iter_type(exe, exe.uchar_type)), in(in), i(in->begin())
  { }
  
  opt<Box> UStrIter::next(Scope &scp){
    if (i == in->end()) { return nullopt; }
    auto res(*i);
    i++;
    return Box(exec.uchar_type, res);
  }

  static void str_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<StrRef>(in)->size());
  }

  static void str_bytes_imp(Scope &scp, const Args &args) {
    auto &in(get<StrRef>(args.at(0)));
    push(scp.thread,
	 scp.exec.bin_type,
	 std::make_shared<Bin>(in->begin(), in->end()));
  }

  static void bin_str_imp(Scope &scp, const Args &args) {
    auto &in(*get<BinRef>(args.at(0)));
    push(scp.thread, scp.exec.str_type, std::make_shared<str>(in.begin(), in.end()));
  }

  static void str_ustr_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.ustr_type,
	 std::make_shared<ustr>(uconv.from_bytes(*get<StrRef>(args.at(0)))));
  }

  static void ustr_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<UStrRef>(in)->size());
  }

  static void ustr_bytes_imp(Scope &scp, const Args &args) {
    auto in(uconv.to_bytes(*get<UStrRef>(args.at(0))));
    push(scp.thread, scp.exec.bin_type, std::make_shared<Bin>(in.begin(), in.end()));
  }

  static void bin_ustr_imp(Scope &scp, const Args &args) {
    auto &in(*get<BinRef>(args.at(0)));
    push(scp.thread, scp.exec.ustr_type,
	 std::make_shared<ustr>(uconv.from_bytes(str(in.begin(), in.end()))));
  }

  static void ustr_str_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.ustr_type,
	 std::make_shared<str>(uconv.to_bytes(*get<UStrRef>(args.at(0)))));
  }

  static void i64_str_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.str_type,
	 std::make_shared<str>(std::to_string(get<int64_t>(args.at(0)))));
  }

  static void iterable_str_imp(Scope &scp, const Args &args) {    
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    str out;
    
    while (true) {
      auto c(it->next(scp));
      if (!c) { break; }
      out.push_back(get<char>(*c));
    }
    
    push(scp.thread, scp.exec.str_type, std::make_shared<str>(out)); 
  }

  static void iterable_join_str_imp(Scope &scp, const Args &args) {    
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto &sep(args.at(1));
    OutStream out;
    bool first(true);
    
    while (true) {
      auto v(it->next(scp));
      if (!v) { break; }
      if (!first) { out << sep.type->fmt(sep); }
      out << v->type->fmt(*v);
      first = false;
    }
    
    push(scp.thread, scp.exec.str_type, std::make_shared<str>(out.str())); 
  }

  static void iterable_lines_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, get_opt_type(exe, exe.str_type)),
	 IterRef(new SplitIter(exe, (*in.type->iter)(in), [](auto &c) {
	       return c == '\r' || c == '\n';
	     })));
  }

  static void iterable_words_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, get_opt_type(exe, exe.str_type)),
	 IterRef(new SplitIter(exe, (*in.type->iter)(in), [](auto &c) {
	       return !isalpha(c);
	     })));
  }
  
  void init_strs(Exec &exe) {
    exe.str_type.supers.push_back(&exe.any_type);
    exe.str_type.supers.push_back(&exe.ordered_type);
    exe.str_type.supers.push_back(&get_iterable_type(exe, exe.char_type));
    exe.str_type.fmt = [](auto &v) { return *get<StrRef>(v); };
    exe.str_type.dump = [](auto &v) { return fmt("'%0'", *get<StrRef>(v)); };

    exe.str_type.eq = [](auto &x, auto &y) {
      return get<StrRef>(x) == get<StrRef>(y);
    };

    exe.str_type.equal = [](auto &x, auto &y) {
      return *get<StrRef>(x) == *get<StrRef>(y);
    };
    
    exe.str_type.lt = [](auto &x, auto &y) {
      return *get<StrRef>(x) < *get<StrRef>(y);
    };

    exe.str_type.iter = [&exe](auto &in) {
      return IterRef(new StrIter(exe, get<StrRef>(in)));
    };

    exe.ustr_type.supers.push_back(&exe.any_type);
    exe.ustr_type.supers.push_back(&exe.ordered_type);
    exe.ustr_type.supers.push_back(&get_iterable_type(exe, exe.uchar_type));

    exe.ustr_type.fmt = [](auto &v) {
      return fmt("u'%0'", uconv.to_bytes(*get<UStrRef>(v)));
    };

    exe.ustr_type.eq = [](auto &x, auto &y) {
      return get<UStrRef>(x) == get<UStrRef>(y);
    };

    exe.ustr_type.equal = [](auto &x, auto &y) {
      return *get<UStrRef>(x) == *get<UStrRef>(y);
    };

    exe.ustr_type.lt = [](auto &x, auto &y) {
      return *get<UStrRef>(x) < *get<UStrRef>(y);
    };

    exe.ustr_type.iter = [&exe](auto &in) {
      return IterRef(new UStrIter(exe, get<UStrRef>(in)));
    };

    add_func(exe, "len", {ArgType(exe.str_type)}, str_len_imp);
    add_func(exe, "bytes", {ArgType(exe.str_type)}, str_bytes_imp);
    add_func(exe, "str", {ArgType(exe.bin_type)}, bin_str_imp);
    add_func(exe, "ustr", {ArgType(exe.str_type)}, str_ustr_imp);

    add_func(exe, "len", {ArgType(exe.ustr_type)}, ustr_len_imp);
    add_func(exe, "bytes", {ArgType(exe.ustr_type)}, ustr_bytes_imp);
    add_func(exe, "ustr", {ArgType(exe.bin_type)}, bin_ustr_imp);
    add_func(exe, "str", {ArgType(exe.ustr_type)}, ustr_str_imp);

    add_func(exe, "str", {ArgType(exe.i64_type)}, i64_str_imp);

    add_func(exe, "str",
	     {ArgType(get_iterable_type(exe, exe.char_type))},
	     iterable_str_imp);

    add_func(exe, "join",
	     {ArgType(exe.iterable_type), ArgType(exe.any_type)},
	     iterable_join_str_imp);

    add_func(exe, "lines",
	     {ArgType(get_iterable_type(exe, exe.bin_type))},
	     iterable_lines_imp);

    add_func(exe, "words",
	     {ArgType(get_iterable_type(exe, exe.bin_type))},
	     iterable_words_imp);

    add_conv(exe, exe.str_type, exe.ustr_type, [&exe](auto &v) {	
	v.type = &exe.ustr_type;
	v.val = std::make_shared<ustr>(uconv.from_bytes(*get<StrRef>(v)));
	return true;
      });
  }
}
