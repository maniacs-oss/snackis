#include <chrono>
#include <iostream>

#include "snabel/compiler.hpp"
#include "snabel/exec.hpp"
#include "snabel/op.hpp"
#include "snabel/parser.hpp"
#include "snabel/type.hpp"
#include "snackis/core/error.hpp"

namespace snabel {
  static void test_func(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    CHECK(args.size() == 1, _);
    CHECK(args[0].type == &exe.i64_type, _);
    push(scp.coro, exe.i64_type, 35+get<int64_t>(args[0]));
  }
  
  static void func_tests() {
    TRY(try_test);
    Exec exe;
    
    run(exe, "func: foo1 {35 +}; 7 foo1");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe,
	"func: foo2 {35 +};"
	"let: bar &foo2;"
	"7 $bar call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void parse_lines_tests() {
    TRY(try_test);    
    auto ls(parse_lines("foo\nbar\nbaz"));
    CHECK(ls.size() == 3, _);
    CHECK(ls[0] == "foo", _);
    CHECK(ls[1] == "bar", _);
    CHECK(ls[2] == "baz", _);
  }

  static void parse_semicolon_tests() {
    TRY(try_test);    
    auto ts(parse_expr(";foo; bar baz;"));
    CHECK(ts.size() == 6, _);
    CHECK(ts[0].text == ";", _);
    CHECK(ts[1].text == "foo", _);
    CHECK(ts[2].text == ";", _);
    CHECK(ts[3].text == "bar", _);
    CHECK(ts[4].text == "baz", _);
    CHECK(ts[5].text == ";", _);
  }

  static void parse_braces_tests() {
    TRY(try_test);    
    auto ts(parse_expr("{ foo } {}; bar"));
    CHECK(ts.size() == 7, _);
    CHECK(ts[0].text == "{", _);
    CHECK(ts[1].text == "foo", _);
    CHECK(ts[2].text == "}", _);

    CHECK(ts[3].text == "{", _);
    CHECK(ts[4].text == "}", _);
    
    CHECK(ts[5].text == ";", _);
    CHECK(ts[6].text == "bar", _);
  }

  static void parse_string_tests() {
    auto ts(parse_expr("'foo ' 1 2"));

    CHECK(ts.size() == 3, _);
    CHECK(ts[0].text == "'foo '", _);
    CHECK(ts[1].text == "1", _);
    CHECK(ts[2].text == "2", _);

    ts = parse_expr("1 'foo' 2");
    CHECK(ts.size() == 3, _);
    CHECK(ts[0].text == "1", _);
    CHECK(ts[1].text == "'foo'", _);
    CHECK(ts[2].text == "2", _);

    ts = parse_expr("let: foo {42}; '$foo'");
    CHECK(ts.size() == 7, _);
    CHECK(ts[0].text == "let:", _);
    CHECK(ts[1].text == "foo", _);
    CHECK(ts[2].text == "{", _);
    CHECK(ts[3].text == "42", _);
    CHECK(ts[4].text == "}", _);
    CHECK(ts[5].text == ";", _);
    CHECK(ts[6].text == "'$foo'", _);
  }

  static void parse_list_tests() {
    auto ts(parse_expr("[42 'bar']"));

    CHECK(ts.size() == 4, _);
    CHECK(ts[0].text == "[", _);
    CHECK(ts[1].text == "42", _);
    CHECK(ts[2].text == "'bar'", _);
    CHECK(ts[3].text == "]", _);
  }

  static void parse_tests() {
    parse_lines_tests();

    auto ts(parse_expr("1"));	
    CHECK(ts.size() == 1, _);
    CHECK(ts[0].text == "1", _);
    
    parse_semicolon_tests();
    parse_braces_tests();
    parse_string_tests();
    parse_list_tests();
  }

  static void parens_tests() {
    TRY(try_test);    
    auto ts(parse_expr("foo (bar (35 7)) baz"));
    
    CHECK(ts.size() == 9, _);
    CHECK(ts[0].text == "foo", _);
    CHECK(ts[1].text == "(", _);
    CHECK(ts[2].text == "bar", _);
    CHECK(ts[3].text == "(", _);
    CHECK(ts[4].text == "35", _);
    CHECK(ts[5].text == "7", _);
    CHECK(ts[6].text == ")", _);
    CHECK(ts[7].text == ")", _);
    CHECK(ts[8].text == "baz", _);

    Exec exe;
    run(exe, "(1 1 +) (2 2 +) *");
    CHECK(get<int64_t>(pop(exe.main)) == 8, _);    
  }

  static void compile_tests() {
    TRY(try_test);
    Exec exe;
    run(exe, "let: foo 35; let: bar $foo 7 +");

    Scope &scp(curr_scope(exe.main));
    CHECK(get<int64_t>(get_env(scp, "$foo")) == 35, _);
    CHECK(get<int64_t>(get_env(scp, "$bar")) == 42, _);
  }

  static void type_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "42 I64 is?");
    CHECK(get<bool>(pop(exe.main)), _);

    run(exe, "42 type");
    CHECK(get<Type *>(pop(exe.main)) == &exe.i64_type, _);

    run(exe, "Iter<>");
    CHECK(get<Type *>(pop(exe.main)) == &exe.iter_type, _);

    run(exe, "List<Str>");
    CHECK(get<Type *>(pop(exe.main)) == &get_list_type(exe, exe.str_type), _);

    run(exe, "Pair<Str>");
    CHECK(get<Type *>(pop(exe.main)) ==
	  &get_pair_type(exe, exe.str_type, exe.any_type), _);

    run(exe, "(List)<I64>");
    CHECK(get<Type *>(pop(exe.main)) == &get_list_type(exe, exe.i64_type), _);

    run(exe, "I64 list List<I64>!");
    CHECK(pop(exe.main).type == &get_list_type(exe, exe.i64_type), _);
  }

  static void stack_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "42 reset");
    CHECK(!peek(exe.main), _);

    compile(exe, "1 2 3 _ _");
    CHECK(exe.main_thread.ops.size() == 1, _);
  }

  static void scope_tests() {
    TRY(try_test);    
    Exec exe;
    
    run(exe, "(let: foo 21;$foo)");
    Scope &scp1(curr_scope(exe.main));
    CHECK(get<int64_t>(pop(scp1.coro)) == 21, _);
    CHECK(!find_env(scp1, "foo"), _);
  }

  static void equality_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "[1 2 3] [1 2 3] =");
    CHECK(!get<bool>(pop(exe.main)), _);

    run(exe, "[1 2 3] [1 2 3] ==");
    CHECK(get<bool>(pop(exe.main)), _);
  }

  static void let_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "let: foo 35 7 +; $foo");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void jump_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "3 4 exit 35 @exit +");
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);
  }

  static void lambda_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "{3 4 +} call 35 +");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe, "2 {3 +} call");
    CHECK(get<int64_t>(pop(exe.main)) == 5, _);

    run(exe, "{let: bar 42; $bar} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
    CHECK(!find_env(curr_scope(exe.main), "bar"), _);

    run(exe, "let: fn {7 +}; 35 $fn call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe, "{7 35 + return 99} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe, "{#t {42} when} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe, "#t {$0 &return when #f} call");
    CHECK(get<bool>(pop(exe.main)), _);

    run(exe, "42 {dec $0 zero? &return when recall} call");
    CHECK(get<int64_t>(pop(exe.main)) == 0, _);
  }

  static void when_tests() {
    TRY(try_test);    
    Exec exe;
    
    run(exe, "7 #t {35 +} when");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run(exe, "7 #f {35 +} when");
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);
  }

  static void list_push_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "[0] 1 push");
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 2, _);
  }

  static void list_pop_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "[7 35] pop");
    
    CHECK(get<int64_t>(pop(exe.main)) == 35, _);
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 1, _);
    CHECK(get<int64_t>(ls->front()) == 7, _);
  }

  static void list_reverse_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "[0 1 2] reverse");
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 3, _);
    
    for (int64_t i(0); i < 3; i++) {
      CHECK(get<int64_t>(ls->at(i)) == 2-i, _);
    }
  }

  static void iter_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "7 \\, join");
    CHECK(get<str>(pop(exe.main)) == "0,1,2,3,4,5,6", _);

    run(exe, "3 iter pop $1 pop $1 pop $1 _ + +");
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);
    
    run(exe, "let: foo 'bar' iter; $foo list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 3, _);

    run(exe, "7 iter 7 iter =");
    CHECK(!get<bool>(pop(exe.main)), _);

    run(exe, "7 iter 7 iter ==");
    CHECK(get<bool>(pop(exe.main)), _);

    run(exe, "7 list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 7, _);

    run(exe, "[1 2 3] ['foo' 'bar'] zip list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 2, _);
  }
  
  static void list_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "[0 1 2]");
    auto lsb(pop(exe.main));
    CHECK(lsb.type == &get_list_type(exe, exe.i64_type), _);
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 3, _);
    
    for (int64_t i(0); i < 3; i++) {
      CHECK(get<int64_t>(ls->at(i)) == i, _);
    }

    run(exe, "Str list 'foo' push");
    CHECK(get<ListRef>(pop(exe.main))->size() == 1, _);

    run(exe, "['foo' 'bar'] 7 list zip list unzip _ list");
    CHECK(get<str>(get<ListRef>(pop(exe.main))->back()) == "bar", _);

    run(exe, "['foo' 7. 'bar' 35.] unzip _ list");
    CHECK(get<str>(get<ListRef>(pop(exe.main))->back()) == "bar", _);

    list_push_tests();
    list_pop_tests();
    list_reverse_tests();
  }

  static void pair_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "42 'foo' .");
    auto p(get<PairRef>(pop(exe.main)));
    CHECK(get<int64_t>(p->first) == 42, _);    
    CHECK(get<str>(p->second) == "foo", _);    

    run(exe, "42 'foo'. unzip");
    CHECK(get<str>(pop(exe.main)) == "foo", _);    
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);    
  }

  static void for_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "0 7 &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);

    run(exe, "0 [1 2 3 4 5 6] &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);

    run(exe, "'foo' &nop for stash \\- join");
    CHECK(get<str>(pop(exe.main)) == "f-o-o", _);
  }

  static void rat_tests() {
    TRY(try_test);    
    Exec exe;

    run(exe, "1 2 / 2 3 / + -1 6 / +");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);

    run(exe, "-1 -2 / -1 2 / +");
    CHECK(get<Rat>(pop(exe.main)) == Rat(0, 1), _);

    run(exe, "4 6 / -1 3 / -");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);

    run(exe, "-4 6 / 1 3 / -");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1, true), _);

    run(exe, "2 3 / -1 2 / *");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 3, true), _);

    run(exe, "1 3 / -5 /");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 15, true), _);

    run(exe, "-7 3 / trunc");
    CHECK(get<int64_t>(pop(exe.main)) == -2, _);

    run(exe, "-7 3 / frac");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 3, true), _);

    run(exe,
	"1 1 / "
	"10 {_ 3 /} for "
	"10 {_ 3 *} for");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);
  }
  
  static void thread_tests() {
    TRY(try_test);    
    Exec exe;
    run(exe, "7 {35 +} thread join");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }
  
  static void loop() {
    func_tests();
    parse_tests();
    parens_tests();
    compile_tests();
    type_tests();
    stack_tests();
    scope_tests();
    equality_tests();
    let_tests();
    jump_tests();
    lambda_tests();
    when_tests();
    iter_tests();
    list_tests();
    pair_tests();
    for_tests();
    rat_tests();
    thread_tests();
  }

  void all_tests() {
    TRY(try_snabel);
    const int iters(100), warmups(10);
    for(int i(0); i < warmups; ++i) { loop(); }
    auto started(std::chrono::high_resolution_clock::now());    

    for(int i(0); i < iters; ++i) {
      loop();
      if (i < iters-1) { try_snabel.errors.clear(); }
    }
    
    auto stopped = std::chrono::high_resolution_clock::now();
    auto t =
      std::chrono::duration_cast<std::chrono::microseconds>(stopped-started).count();
    std::cout << "Snabel: " << t / iters << "us" << std::endl;
  }
}
