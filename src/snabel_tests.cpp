#include <chrono>
#include <iostream>

#include "snabel/compiler.hpp"
#include "snabel/exec.hpp"
#include "snabel/list.hpp"
#include "snabel/op.hpp"
#include "snabel/pair.hpp"
#include "snabel/parser.hpp"
#include "snabel/type.hpp"
#include "snackis/core/error.hpp"
#include "snackis/core/time.hpp"

namespace snabel {
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

  static void parse_str_tests() {
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

    ts = parse_expr("let: foo {42}; '@foo'");
    CHECK(ts.size() == 7, _);
    CHECK(ts[0].text == "let:", _);
    CHECK(ts[1].text == "foo", _);
    CHECK(ts[2].text == "{", _);
    CHECK(ts[3].text == "42", _);
    CHECK(ts[4].text == "}", _);
    CHECK(ts[5].text == ";", _);
    CHECK(ts[6].text == "'@foo'", _);
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
    parse_str_tests();
    parse_list_tests();
  }

  Exec exe;

  void run_test(Exec &exe, const str &in) {
    reset(exe);
    compile(exe, in);
    begin_scope(exe.main);
    run(exe.main);
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

    run_test(exe, "(1 1 +) (2 2 +) *");
    CHECK(get<int64_t>(pop(exe.main)) == 8, _);    
  }

  static void compile_tests() {
    TRY(try_test);
    run_test(exe, "let: foo 35; let: bar @foo 7 +");

    Scope &scp(curr_scope(exe.main));
    CHECK(get<int64_t>(get_env(scp, "@foo")) == 35, _);
    CHECK(get<int64_t>(get_env(scp, "@bar")) == 42, _);
  }

  static void func_tests() {
    TRY(try_test);
    run_test(exe, "func: foo {35 +}; 7 foo");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe,
	"func: foo {\n"
        "35 +\n"
	"};\n"
	"let: bar &foo;"
	"7 @bar call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void type_tests() {
    TRY(try_test);    

    run_test(exe, "42 I64 is?");
    CHECK(get<bool>(pop(exe.main)), _);

    run_test(exe, "42 type");
    CHECK(get<Type *>(pop(exe.main)) == &exe.i64_type, _);

    run_test(exe, "Iter<>");
    CHECK(get<Type *>(pop(exe.main)) == &exe.iter_type, _);

    run_test(exe, "List<Str>");
    CHECK(get<Type *>(pop(exe.main)) == &get_list_type(exe, exe.str_type), _);

    run_test(exe, "Pair<Str>");
    CHECK(get<Type *>(pop(exe.main)) ==
	  &get_pair_type(exe, exe.str_type, exe.any_type), _);

    run_test(exe, "(List)<I64>");
    CHECK(get<Type *>(pop(exe.main)) == &get_list_type(exe, exe.i64_type), _);

    run_test(exe, "I64 list List<I64>!");
    CHECK(pop(exe.main).type == &get_list_type(exe, exe.i64_type), _);
  }

  static void stack_tests() {
    TRY(try_test);    

    run_test(exe, "42 _");
    CHECK(!peek(exe.main), _);

    run_test(exe, "7 35 |");
    CHECK(!peek(exe.main), _);
  }

  static void group_tests() {
    TRY(try_test);    
    
    run_test(exe, "(42 let: foo 21; @foo)");
    Scope &scp1(curr_scope(exe.main));
    CHECK(get<int64_t>(pop(scp1.thread)) == 21, _);
    CHECK(!peek(scp1.thread), _);
    CHECK(get<int64_t>(get_env(scp1, "@foo")) == 21, _);
  }

  static void equality_tests() {
    TRY(try_test);    

    run_test(exe, "[1 2 3] [1 2 3] =");
    CHECK(!get<bool>(pop(exe.main)), _);

    run_test(exe, "[1 2 3] [1 2 3] ==");
    CHECK(get<bool>(pop(exe.main)), _);
  }

  static void let_tests() {
    TRY(try_test);    
    run_test(exe, "let: foo 35 7 +; @foo");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void lambda_tests() {
    TRY(try_test);    

    run_test(exe, "{3 4 +} call 35 +");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "2 {3 +} call");
    CHECK(get<int64_t>(pop(exe.main)) == 5, _);

    run_test(exe, "{let: bar 42; @bar} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
    CHECK(!find_env(curr_scope(exe.main), "bar"), _);

    run_test(exe, "let: fn {7 +}; 35 @fn call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "{7 35 + return 99} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "{true {42} when} call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "true {$ &return when false} call");
    CHECK(get<bool>(pop(exe.main)), _);

    run_test(exe, "42 {1 - $ z? &return when recall 2 +} call");
    CHECK(get<int64_t>(pop(exe.main)) == 82, _);

    run_test(exe, "42 {1 - $ z? &return when (2 (|recall) +)} call");
    CHECK(get<int64_t>(pop(exe.main)) == 82, _);
  }

  static void coro_tests() {
    TRY(try_test);    
    
    run_test(exe,
	"func: foo {|yield (7 yield 28 +)} call; "
	"foo foo +");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe,
	"func: foo {|yield (let: bar 28; 7 yield @bar +)} call; "
	"foo foo +");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe,
	"func: foo {|yield ([7 35] &yield for &+)} call; "
	"foo foo foo call");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe,
	"let: acc I64 list; "

	"func: foo {|yield ( "
	"  7 {@acc $1 push yield1} for "
	")} call; "

	"7 &foo for "
	"0 @acc &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);
  }
  
  static void cond_tests() {
    TRY(try_test);    
    
    run_test(exe, "7 true {35 +} when");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "7 false {35 +} when");
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);

    run_test(exe, "7 true {35 +} unless");
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);

    run_test(exe, "7 false {35 +} unless");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void sym_tests() {
    TRY(try_test);    
    
    run_test(exe, "#foo");
    CHECK(name(get<Sym>(pop(exe.main))) == "foo", _);

    run_test(exe, "#foo $ =");
    CHECK(get<bool>(pop(exe.main)), _);

    run_test(exe, "#foo 'foo' sym =");
    CHECK(get<bool>(pop(exe.main)), _);

    run_test(exe, "#foo #bar =");
    CHECK(!get<bool>(pop(exe.main)), _);

    run_test(exe, "#foo #bar gt?");
    CHECK(get<bool>(pop(exe.main)), _);
  }

  static void str_tests() {
    TRY(try_test);    
    
    run_test(exe, "'foo' len");
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);

    run_test(exe,
	"['foo\\r\\n\\r\\nbar\\r\\n\\r\\nbaz' bytes]"
	"lines unopt \\, join");	
    CHECK(get<str>(pop(exe.main)) == "foo,bar,baz", _);
  }

  static void bin_tests() {
    TRY(try_test);    
    
    run_test(exe, "3 bytes len");
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);

    run_test(exe, "'foo' bytes str");
    CHECK(get<str>(pop(exe.main)) == "foo", _);

    run_test(exe, "'foo' bytes 'bar' bytes append str");
    CHECK(get<str>(pop(exe.main)) == "foobar", _);

    run_test(exe, "u'foo' bytes str");
    CHECK(get<str>(pop(exe.main)) == "foo", _);
  }

  static void uid_tests() {
    TRY(try_test);    
    
    run_test(exe, "uid uid ==");
    CHECK(!get<bool>(pop(exe.main)), _);
  }

  static void list_push_tests() {
    TRY(try_test);    

    run_test(exe, "[0] 1 push");
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 2, _);
  }

  static void list_pop_tests() {
    TRY(try_test);    

    run_test(exe, "[7 35] pop");    
    CHECK(get<int64_t>(pop(exe.main)) == 35, _);
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 1, _);
    CHECK(get<int64_t>(ls->front()) == 7, _);
  }

  static void list_reverse_tests() {
    TRY(try_test);    

    run_test(exe, "[0 1 2] reverse");
    auto lsb(pop(exe.main));
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 3, _);
    
    for (int64_t i(0); i < 3; i++) {
      CHECK(get<int64_t>(ls->at(i)) == 2-i, _);
    }
  }

  static void iter_tests() {
    TRY(try_test);    

    run_test(exe, "7 \\, join");
    CHECK(get<str>(pop(exe.main)) == "0,1,2,3,4,5,6", _);

    run_test(exe, "let: foo 'bar' iter; @foo list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 3, _);

    run_test(exe, "7 iter 7 iter =");
    CHECK(!get<bool>(pop(exe.main)), _);

    run_test(exe, "7 list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 7, _);

    run_test(exe, "[1 2 3] ['foo' 'bar'] zip list");
    CHECK(get<ListRef>(pop(exe.main))->size() == 2, _);

    run_test(exe, "[1 2 3] {7 *} map &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "'abcabcabc' {\\a =} filter str");
    CHECK(get<str>(pop(exe.main)) == "aaa", _);
  }
  
  static void list_tests() {
    TRY(try_test);    

    run_test(exe, "[0 1 2]");
    auto lsb(pop(exe.main));
    CHECK(lsb.type == &get_list_type(exe, exe.i64_type), _);
    auto ls(get<ListRef>(lsb));
    CHECK(ls->size() == 3, _);
    
    for (int64_t i(0); i < 3; i++) {
      CHECK(get<int64_t>(ls->at(i)) == i, _);
    }

    run_test(exe, "Str list 'foo' push");
    CHECK(get<ListRef>(pop(exe.main))->size() == 1, _);

    run_test(exe, "['foo' 'bar'] 7 list zip list unzip _ list");
    CHECK(get<str>(get<ListRef>(pop(exe.main))->back()) == "bar", _);

    run_test(exe, "['foo' 7. 'bar' 35.] unzip _ list");
    CHECK(get<str>(get<ListRef>(pop(exe.main))->back()) == "bar", _);

    run_test(exe, "[2 3 1] &lt? sort pop");
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);
      
    list_push_tests();
    list_pop_tests();
    list_reverse_tests();
  }

  static void pair_tests() {
    TRY(try_test);    

    run_test(exe, "42 'foo'.");
    auto p(get<PairRef>(pop(exe.main)));
    CHECK(get<int64_t>(p->first) == 42, _);    
    CHECK(get<str>(p->second) == "foo", _);    

    run_test(exe, "'foo' 42. left");
    CHECK(get<str>(pop(exe.main)) == "foo", _);    
    
    run_test(exe, "'foo' 42. right");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);    
    
    run_test(exe, "42 'foo'. unzip");
    CHECK(get<str>(pop(exe.main)) == "foo", _);    
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);    
  }

  static void loop_tests() {
    TRY(try_test);    
    
    run_test(exe, "0 7 &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);

    run_test(exe, "0 7 {$ 5 = {_ break} when +} for");
    CHECK(get<int64_t>(pop(exe.main)) == 10, _);

    run_test(exe, "0 7 {$ 5 = &break when +} for _");
    CHECK(get<int64_t>(pop(exe.main)) == 10, _);

    run_test(exe, "0 7 {$ 1 {_ 5 = {break} when} for +} for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);

    run_test(exe, "0 7 {$ 1 {_ 5 = {_ break1} when +} for} for");
    CHECK(get<int64_t>(pop(exe.main)) == 10, _);

    run_test(exe, "0 [1 2 3 4 5 6] &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);

    run_test(exe, "'foo' &nop for $list \\- join");
    CHECK(get<str>(pop(exe.main)) == "f-o-o", _);

    run_test(exe, "0 {$ 42 lt?} {1 +} while");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void rat_tests() {
    TRY(try_test);    

    run_test(exe, "1 2 / 2 3 / + -1 6 / +");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);

    run_test(exe, "-1 -2 / -1 2 / +");
    CHECK(get<Rat>(pop(exe.main)) == Rat(0, 1), _);

    run_test(exe, "4 6 / -1 3 / -");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);

    run_test(exe, "-4 6 / 1 3 / -");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1, true), _);

    run_test(exe, "2 3 / -1 2 / *");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 3, true), _);

    run_test(exe, "1 3 / -5 /");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 15, true), _);

    run_test(exe, "-7 3 / trunc");
    CHECK(get<int64_t>(pop(exe.main)) == -2, _);

    run_test(exe, "-7 3 / frac");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 3, true), _);

    run_test(exe,
	"1 1 / "
	"10 {_ 3 /} for "
	"10 {_ 3 *} for");
    CHECK(get<Rat>(pop(exe.main)) == Rat(1, 1), _);
  }

  static void opt_tests() {
    TRY(try_test);    

    run_test(exe, "42 opt");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "nil 42 or");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe, "7 opt 42 opt or");
    CHECK(get<int64_t>(pop(exe.main)) == 7, _);

    run_test(exe, "0 [7 opt nil 35 opt] unopt &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
  }

  static void io_tests() {
    TRY(try_test);    

    run_test(exe, "0 '../dist/snackis' rfile read {{len $1 _ +} when} for");
    CHECK(get<int64_t>(pop(exe.main)) > 3000000, _);

    run_test(exe, "['foo' bytes] 'tmp' rwfile write 0 $1 &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 3, _);
  }
  
  static void proc_tests() {
    TRY(try_test);    
    
    run_test(exe,
	"func: foo {|yield (yield 35 push)} call proc;"
	"0 [7] foo foo iter &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);

    run_test(exe,
	"let: acc Str list; "
	"func: ping {|yield (3 {@acc 'ping' push yield1} for)} call proc; "
	"func: pong {|yield (3 {@acc 'pong' push yield1} for)} call proc; "
	"[&ping &pong] run &_ for");
    CHECK(get<ListRef>(get_env(exe.main_scope, "@acc"))->size() == 6, _);
  
    run_test(exe,
	"let: acc I64 list; "

	"let: foo {|yield ( "
	"  7 {@acc $1 push _ yield1} for "
	")} call proc; "

	"@foo run "
	"0 @acc &+ for");
    CHECK(get<int64_t>(pop(exe.main)) == 21, _);
  }

  /*
  static void thread_tests() {
    TRY(try_test);    
    Exec exe;
    run_test(exe, "7 {35 +} thread join");
    CHECK(get<int64_t>(pop(exe.main)) == 42, _);
    }*/
  
  static void loop() {
    parse_tests();
    parens_tests();
    compile_tests();
    func_tests();
    type_tests();
    stack_tests();
    group_tests();
    equality_tests();
    let_tests();
    lambda_tests();
    coro_tests();
    cond_tests();
    sym_tests();
    str_tests();
    bin_tests();
    uid_tests();
    iter_tests();
    list_tests();
    pair_tests();
    loop_tests();
    rat_tests();
    opt_tests();
    io_tests();
    proc_tests();
    //thread_tests();
  }

  void all_tests() {
    TRY(try_snabel);
    const int iters(100), warmups(10);

    for(int i(0); i < warmups; ++i) { loop(); }
    
    auto started(pnow());    

    for(int i(0); i < iters; ++i) {
      loop();
      if (i < iters-1) { try_snabel.errors.clear(); }
    }
    
    std::cout << "Snabel: " << usecs(pnow()-started) / iters << "us" << std::endl;
  }
}
