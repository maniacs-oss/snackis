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

  static void len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, scp.exec.i64_type, (int64_t)get<StrRef>(in)->size());
  }

  static void suffix_imp(Scope &scp, const Args &args) {
    auto &in(*get<StrRef>(args.at(0))), &x(*get<StrRef>(args.at(1)));
    push(scp.thread, scp.exec.bool_type, suffix(in, x));
  }

  static void upcase_imp(Scope &scp, const Args &args) {
    upcase(*get<StrRef>(args.at(0)));
  }

  static void downcase_imp(Scope &scp, const Args &args) {
    downcase(*get<StrRef>(args.at(0)));
  }

  static void reverse_imp(Scope &scp, const Args &args) {
    auto &in(*get<StrRef>(args.at(0)));
    std::reverse(in.begin(), in.end());
  }

  static void str_push_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    get<StrRef>(in)->push_back(get<char>(args.at(1)));
  }

  static void clear_imp(Scope &scp, const Args &args) {
    get<StrRef>(args.at(0))->clear();
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

  static void ulen_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, scp.exec.i64_type, (int64_t)get<UStrRef>(in)->size());
  }

  static void usuffix_imp(Scope &scp, const Args &args) {
    auto &in(*get<UStrRef>(args.at(0))), &x(*get<UStrRef>(args.at(1)));
    push(scp.thread, scp.exec.bool_type, suffix(in, x));
  }

  static void ureverse_imp(Scope &scp, const Args &args) {
    auto &in(*get<UStrRef>(args.at(0)));
    std::reverse(in.begin(), in.end());
  }

  static void ustr_push_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    get<UStrRef>(in)->push_back(get<uchar>(args.at(1)));
  }

  static void uclear_imp(Scope &scp, const Args &args) {
    get<UStrRef>(args.at(0))->clear();
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

  static void stoi64_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.i64_type, to_int64(*get<StrRef>(args.at(0))));
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
	       return c != '\'' && !isalpha(c);
	     })));
  }
  
  void init_strs(Exec &exe) {
    exe.char_type.supers.push_back(&exe.any_type);
    exe.char_type.supers.push_back(&exe.ordered_type);

    exe.char_type.dump = [](auto &v) -> str {
      auto c(get<char>(v));
      
      switch (c) {
      case ' ':
	return "\\space";
      case '\n':
	return "\\n";
      case '\t':
	return "\\t";
      }

      return fmt("\\%0", str(1, c));
    };

    exe.char_type.fmt = [](auto &v) -> str { return str(1, get<char>(v)); };
    exe.char_type.eq = [](auto &x, auto &y) { return get<char>(x) == get<char>(y); };
    exe.char_type.lt = [](auto &x, auto &y) { return get<char>(x) < get<char>(y); };
    
    exe.uchar_type.supers.push_back(&exe.any_type);
    exe.uchar_type.supers.push_back(&exe.ordered_type);
    
    exe.uchar_type.dump = [](auto &v) -> str {
      auto c(get<uchar>(v));
      
      switch (c) {
      case u' ':
	return "\\\\space";
      case u'\n':
	return "\\\\n";
      case u'\t':
	return "\\\\t";
      }

      return fmt("\\\\%0", uconv.to_bytes(ustr(1, c)));
    };

    exe.uchar_type.fmt = [](auto &v) -> str {
      return uconv.to_bytes(ustr(1, get<uchar>(v)));
    };
    
    exe.uchar_type.eq = [](auto &x, auto &y) {
      return get<uchar>(x) == get<uchar>(y);
    };
    
    exe.uchar_type.lt = [](auto &x, auto &y) {
      return get<uchar>(x) < get<uchar>(y);
    };

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
      return fmt("\"%0\"", uconv.to_bytes(*get<UStrRef>(v)));
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

    add_func(exe, "len", {ArgType(exe.str_type)}, len_imp);
    
    add_func(exe, "suffix?",
	     {ArgType(exe.str_type), ArgType(exe.str_type)},
	     suffix_imp);

    add_func(exe, "upcase", {ArgType(exe.str_type)}, upcase_imp);
    add_func(exe, "downcase", {ArgType(exe.str_type)}, downcase_imp);
    add_func(exe, "reverse", {ArgType(exe.str_type)}, reverse_imp);

    add_func(exe, "push",
	     {ArgType(exe.str_type), ArgType(exe.char_type)},
	     str_push_imp);

    add_func(exe, "clear", {ArgType(exe.str_type)}, clear_imp);
    add_func(exe, "bytes", {ArgType(exe.str_type)}, str_bytes_imp);
    add_func(exe, "str", {ArgType(exe.bin_type)}, bin_str_imp);
    add_func(exe, "ustr", {ArgType(exe.str_type)}, str_ustr_imp);
    add_func(exe, "len", {ArgType(exe.ustr_type)}, ulen_imp);

    add_func(exe, "suffix?",
	     {ArgType(exe.ustr_type), ArgType(exe.ustr_type)},
	     usuffix_imp);

    add_func(exe, "reverse", {ArgType(exe.ustr_type)}, ureverse_imp);

    add_func(exe, "push",
	     {ArgType(exe.ustr_type), ArgType(exe.uchar_type)},
	     ustr_push_imp);

    add_func(exe, "clear", {ArgType(exe.ustr_type)}, uclear_imp);
    add_func(exe, "bytes", {ArgType(exe.ustr_type)}, ustr_bytes_imp);
    add_func(exe, "ustr", {ArgType(exe.bin_type)}, bin_ustr_imp);
    add_func(exe, "str", {ArgType(exe.ustr_type)}, ustr_str_imp);

    add_func(exe, "str", {ArgType(exe.i64_type)}, i64_str_imp);
    add_func(exe, "stoi64", {ArgType(exe.str_type)}, stoi64_imp);

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
