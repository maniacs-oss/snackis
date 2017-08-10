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
    push(scp.coro, exe.i64_type, 42-get<int64_t>(args[0]));
  }
  
  static void func_tests() {
    TRY(try_test);
    Exec exe;
    auto &f(add_func(exe, "test-func", {&exe.i64_type}, exe.i64_type, test_func));
    Coro &cor(exe.main);
    cor.ops.emplace_back(Push(Box(exe.i64_type, int64_t(7))));
    cor.ops.emplace_back(Funcall(f.func));
    run(exe.main);

    CHECK(get<int64_t>(pop(exe.main)) == 35, _);
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
    auto ts(parse_expr("\"foo \" 1 +"));

    CHECK(ts.size() == 3, _);
    CHECK(ts[0].text == "\"foo \"", _);
    CHECK(ts[1].text == "1", _);
    CHECK(ts[2].text == "+", _);

    ts = parse_expr("1 \"foo\" +");
    CHECK(ts.size() == 3, _);
    CHECK(ts[0].text == "1", _);
    CHECK(ts[1].text == "\"foo\"", _);
    CHECK(ts[2].text == "+", _);
  }

  static void parse_list_tests() {
    auto ts(parse_expr("[42 \"bar\"]"));

    CHECK(ts.size() == 4, _);
    CHECK(ts[0].text == "[", _);
    CHECK(ts[1].text == "42", _);
    CHECK(ts[2].text == "\"bar\"", _);
    CHECK(ts[3].text == "]", _);
  }

  static void parse_tests() {
    parse_lines_tests();
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
    compile(exe.main, "(1 1 +) (2 2 +) *");
    run(exe.main);
    
    CHECK(get<int64_t>(pop(exe.main)) == 8, _);    
  }

  static void compile_tests() {
    TRY(try_test);
    Exec exe;
    compile(exe.main, "let: foo 35\nlet: bar $foo 7 +");
    run(exe.main);

    Scope &scp(curr_scope(exe.main));
    CHECK(get<int64_t>(get_env(scp, "$foo")) == 35, _);
    CHECK(get<int64_t>(get_env(scp, "$bar")) == 42, _);
  }

  static void stack_tests() {
    TRY(try_test);    
    Exec exe;
    compile(exe.main, "42 reset");
    run(exe.main);
    CHECK(!peek(exe.main), _);

    compile(exe.main, "1 2 3 drop drop");
    CHECK(exe.main.ops.size() == 1, _);
  }

  static void scope_tests() {
    TRY(try_test);    
    Exec exe;
    
    compile(exe.main, "(let: foo 21;$foo)");
    run(exe.main);
    Scope &scp1(curr_scope(exe.main));
    CHECK(get<int64_t>(pop(scp1.coro)) == 21, _);
    CHECK(!find_env(scp1, "foo"), _);

    compile(exe.main, "begin\nlet: bar 42\n$bar\nend call");
    run(exe.main);
    Scope &scp2(curr_scope(exe.main));
    CHECK(get<int64_t>(pop(scp2.coro)) == 42, _);
    CHECK(!find_env(scp2, "bar"), _);
  }

  static void let_tests() {
    TRY(try_test);    
    Exec exe;
    compile(exe.main, "let: foo 35 7 +; $foo");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void jump_tests() {
    TRY(try_test);    
    Exec exe;
    compile(exe.main, "1 2 exit! 3 @exit +");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);
  }

  static void lambda_tests() {
    TRY(try_test);    
    Exec exe;
    compile(exe.main, "{1 2 +} call");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);

    compile(exe.main, "2 {3 +} call");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 5, _);

    compile(exe.main, "let: fn {7 +}; 35 $fn call");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void when_tests() {
    TRY(try_test);    
    Exec exe;
    
    compile(exe.main, "7 't {35 +} when");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    compile(exe.main, "7 'f {35 +} when");
    run(exe.main);
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);
  }

  static void list_tests() {
    TRY(try_test);    
    Exec exe;
    compile(exe.main, "[0 1 2]");
    run(exe.main);
    auto lsb(pop(exe.main));
    CHECK(lsb.type == &get_list_type(exe, exe.i64_type), _);
    auto ls(get<ListRef>(lsb));
    CHECK(ls->elems.size() == 3, _);
    
    for (int64_t i(0); i < 3; i++) {
      CHECK(get<int64_t>(ls->elems[i]) == i, _);
    }
  }

  void all_tests() {
    func_tests();
    parse_tests();
    parens_tests();
    compile_tests();
    stack_tests();
    scope_tests();
    let_tests();
    jump_tests();
    lambda_tests();
    when_tests();
    list_tests();
  }
}
