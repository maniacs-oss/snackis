# Snabel
#### a decently typed Forth with a touch of Perl

![script example](images/script.png?raw=true)

### Status
Snabel still has quite some way to go before claiming general purpose; I'm a one-man shop and it's development is currently exclusively driven by my needs and interests. I have yet to do any serious comparisons, nor optimizations; but its more than fast enough for scripting.

### Postfix
Like Yoda of Star Wars-fame, and yesterdays scientific calculators; as well as most printers in active use; yet unlike currently trending languages; Snabel expects arguments before operations.

```
> 'Hello World!' say
Hello World!
nil

> 7 35 +
42
```

### The Stack
Values and results from function calls are pushed on the stack in order of appearance. Thanks to lexical scoping and named bindings, keeping the stack squeaky clean is less critical in Snabel. ```$list``` collects all values on the stack in a list that is pushed instead. ```$1```-```$9``` swaps in values, starting from the end; while ```$``` duplicates the last value. ```_``` drops the last value and ```|``` clears the entire stack.

```
> 1 2 3 $list
[1 2 3]

> 1 2 $ $list
[1 2 2]

> 42 7 _ $list
[42]

> 42 7 | $list
[]

> 42 35 $1 -
-7
```

### Expressions
Parentheses may be used to divide expressions into separate parts, each level copies the stack on entry and restores it with the result (if any) pushed on exit.

```
> (1 2 +) (2 2 *) +
7

> 1 (2 3 $list)
[1 2 3]

> 1 (|2 3 $list) .
1 [2 3].
```

### Equality
Two kinds of equality are supported, shallow and deep. Each use a separate operator, ```=``` for shallow comparisons and ```==``` for deep.

```
> [3 4 35] [3 4 35] =
false

> [3 4 35] [3 4 35] ==
true
```

### Bindings
The ```let:```-macro may be used to introduce named bindings. Bound names are prefixed with ```@```, lexically scoped and not allowed to change their value once bound in a specific scope. The bound expression is evaluated in a copy of the calling stack, which allows feeding values into a let statement but avoids unintended stack effects. Bindings require termination using ```;``` to separate them from surrounding code.

```
> let: fn {7 +}; 35 @fn call
42
```

### Types
Types are first class, optionally parameterized and inferred.

```
> I64
I64!

> 42 type
I64!

> [7 35] List<I64> is?
true

> 42 Str!
Check failed, expected Str!
42 I64!
```

#### Strings
Strings are mutable collections of characters that provide reference semantics. Snabel makes a difference between byte strings and unicode strings. Unicode strings are processed as UTF-8 when converting to/from bytes and stored internally as UTF-16. Byte strings are automatically promoted to unicode as needed.

```
> 'foo'
'foo'

> 'foo' 'foo' =
false

> 'foo' 'foo' ==
true

> u'foo'
u'foo'

> 'foo' ustr
u'foo'

> u'foo' 'foo' =
true

> 'ö' len
2

> u'ö' len
1

> ['foo\r\n\r\nbar\r\n\r\nbaz' bytes]
  lines unopt \, join
'foo,bar,baz'
```

#### Symbols
Symbols are unique strings that support efficient comparisons/ordering. Snabel uses symbols internally for all names. Any string may be interned as a symbol, but it makes most sense for smaller sets of short strings that are reused a lot.

```
> #foo
#foo

> #foo $ =
true

> #foo 'foo' sym =
true

> #foo str 'foo' =
true

> #foo #bar =
false

#foo #bar gt?
true
```

#### Rational Numbers
Snabel supports exact arithmetics using rational numbers. Integers are promoted to rationals automatically as needed, while ```trunc``` may be used to convert rationals to integers.

```
> 1 3 /
1/3

> 10 3 / trunc
3

> 1 1 /
  10 {_ 3 /} for
  10 {_ 3 *} for
1/1
```

#### Lists
Lists are based on deques, which means fast inserts/removals in the front/back and decent random access-performance and memory layout. All list types are parameterized by element type, allocate their memory from the heap and provide reference semantics.

```
> [1 2 3]
[1 2 3]

> Str list
[]

> 1 2 3 $list
[1 2 3]

> [35 7 + 'foo']
[42 'foo']

> [1 2] 3 push reverse pop _
[3 2]

> [2 3 1] &lt? sort
[1 2 3]
```

#### Pairs
Pairs have first class support and all iterables support zipping/unzipping. Pairs of values are created using ```.``` while ```zip``` operates on iterables, both values and iterables support ```unzip```.

```
> 'foo' 42.
'foo' 42.

> 'foo' 42. right
42

> 'foo' 42. unzip
'foo' 42

> ['foo' 'bar'] 7 list zip list
['foo' 0. 'bar' 1.]

> ['foo' 0. 'bar' 1.] unzip list $1 list $list
[[0 1] ['foo' 'bar']]
```

#### Optional Values
Optional values are supported through the ```Opt<T>```-type. The empty value is called ```nil```.

```
> 42 opt
Opt(42)

> 7 opt {35 +} when
42

> nil {42} unless
42

> nil 42 or
42

> nil 42 opt or
Opt(42)

> 7 opt 42 opt or
Opt(7)

> [7 opt nil 35 opt]
[Opt(7) nil Opt(35)]

> [7 opt nil 35 opt] unopt list
[7 35]
```

#### Tables
Tables map keys to values, and may be created from any pair iterable. Iterating a table results in a pair iterator.
```
> [7 'foo'. 35 'bar'.] table 35 get
Opt('bar')

> let: acc Str I64 table;
  ['foo,\nbar.baz;\nfoo!' bytes] words
  unopt
  { @acc $1 1 &+1 upsert } for
['bar' 1. 'baz' 1. 'foo' 2.]
```

#### Structs
Structs may be used to create custom composite data types. Constructors and typed field accessors are automatically created. All fields are expected to be initialized when read, reading uninitialized fields signals errors; use optional types for optional fields. An arbitrary number of super types may be listed after the struct name, fields with the same name share storage. Structs are iterable and produce a sequence of symbol/value-pairs. Struct definitions may appear anywhere in the code, types are defined once on compilation and and may be referenced from any scope after the point of definition. When redefining structs, fresh types are created each time; existing instances reference the previous type as do already compiled type literals.

```
> struct: Foo
    a I64
    b Str;
  Foo new Foo is?
true

> Foo b
Str!

> func: make-foo {Foo new 0 set-a '' set-b};
  make-foo
Foo(a 0. b ''.)

> make-foo 42 set-a
Foo(a 42. b ''.)

> make-foo 'bar' set-b list
[#a 0. #b 'bar'.]

> struct: Foo
    c I64;
  make-foo 42 set-c
SnabelError: Function not applicable: set-c
[Foo(a 0. b ''.)]

> Foo new 42 set-c
Foo(c 42.)

> struct: Bar Foo
    d List<Str>;
  Bar new Foo is?
true

> Bar new 42 set-c ['baz'] set-d
Bar(c 42. d ['baz'].)
```

### Lambdas
Wrapping code in braces pushes a pointer on the stack. Lambdas may be exited early by calling ```return```, the final result (if any) is pushed on exit. ```recall``` may be used to call the current lambda recursively. Use ```&return```/```&recall``` to get a target that performs the specified action when called.

```
> {1 2 +}
Lambda(_enter1:0)

> {1 2 +} call
3

> {
    1 2 + return
    14 *
  } call
3

> 42
  {1 - $ z? &return when recall 2 +}
  call
82
```

### Conditions
```when``` accepts a condition and a callable target, the target is called if the condition is true. ```unless``` is the opposite of ```when```.

```
> 7 false {35 +} when
7

> 7 false {35 +} unless
42
```

### Iterators
Many things in snabel are iterable, numbers, strings and lists to name a few. Snabel encourages composing iterables into data pipelines for efficient processing.

```
> 7 \, join
'0,1,2,3,4,5,6'

> [1 2 3] {7 *} map list
[7 14 27]

> 'abcabcabc' {\a =} filter str
"aaa"
```

#### Loops
The ```for```-loop accepts an iterable and a target, and calls the target with the last value pushed on the stack as long as the iterator returns values; and the ```while```-loop accepts a condition and a target, and calls the target as long as the condition pushes ```true```. ```break``` may be used to exit loops early, use ```&break``` to get a target that breaks when called.

```
> 0 7 &+ for
21

> 0 7 {$ 5 = {_ break} when +} for
10

> 0 [1 2 3 4 5 6] &+ for
21

> 'foo' &nop for $list \- join
'f-o-o'

> 0 {$ 42 lt?} {1 +} while
42
```

### Functions
Each function name represents a set of implementations that are matched in reverse declared order when resolving function calls. Prefixing the name of a function with ```&``` pushes it on the stack for later use. Definitions require termination using ```;``` to separate them from surrounding code.

```
> func: foo {35 +}; 7 foo
42

> func: foo {35 +}
  let: bar &foo
  7 @bar call
42
```

#### C++
Adding functions from C++ is as easy as this:

```
using namespace snabel;

static void add_i64(Scope &scp, const Args &args) {
  auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
  push(scp.thread, scp.exec.i64_type, x+y);
}

Exec exe;
add_func(exe, "+",
         {ArgType(exe.i64_type), ArgType(exe.i64_type)},
	 add_i64);
```

### Coroutines
Calling ```yield``` from a lambda logs the current position, stack and environment before returning; execution continues from the yielding position with restored stack and environment on next call. A fresh coroutine context is returned on first ```yield``` when, the context may be used for further calls and will reset itself when the coroutine returns. Use ```&yield``` to get a target that yields when called.

```
> {yield 42} call
Coro(_enter1:1)

> {yield 42} call call
42

> func: foo {|yield (7 yield 28 +)} call;
  foo foo +
42

> func: foo {|yield (let: bar 35; 7 yield @bar +)} call;
  foo foo
42

> func: foo {|yield ([7 35] &yield for &+)} call;
  foo foo foo call
42
```

### Procs
Procs allow interleaving multiple independent flows of control in the same thread. Each proc takes a coroutine as input; when called through a proc, coroutines don't copy arguments or return values.

```
> let: acc Str list;
  func: ping {|yield (3 {@acc 'ping' push yield1} for)} call proc;
  func: pong {|yield (3 {@acc 'pong' push yield1} for)} call proc;
  [&ping &pong] run &_ for
  @acc
['ping' 'pong' 'ping' 'pong 'ping' 'pong']
```

### IO
Snabel provides non-blocking IO in the form of iterators. The provided target is called with each read chunk or written number of bytes pushed on the stack. Files come in two flavors, read-only and read-write.

```
> ['foo' 'bar' 'baz'] &say for
foo
bar
baz
nil

> 'snackis' rfile
RFile(11)

> 'snackis' rfile read
Iter<Bin>

> 0 'snackis' rfile read {{len +} when} for
2313864

> 'tmp' rwfile
RWFile(12)

> 'foo' bytes 'tmp' rwfile write
Iter<I64>

> ['foo' bytes]
  'tmp' rwfile
  write 0 $1 &+ for
3

> func: copy-file {(
    let: q Bin list;
    let: wq @q fifo;
    let: w rwfile @wq $1 write; _
    let: r rfile read; _
    yield

    @r {{
      @q $1 push _
      @w &break _for
    } when yield1} for

    @q +? {@w &_ for} when _
  )};

  'in' 'out' copy-file proc run|
nil

> func: histogram {
    let: max-len; _
    let: min-wlen; _
    let: tbl Str I64 table;

    func: process-file {
      rfile read unopt words unopt
      {len @min-wlen gte? $1 _} filter
      {@tbl $1 1 &+1 upsert _} for
    };

    &process-file for
    @tbl list {right $1 right lt?} sort
    @max-len nlist
  };

  '../src' ls-r &file? filter
  4 100 histogram
['foo' 21. 'bar' 14. 'baz' 7.]
```

### Random Numbers
Random numbers are supported through ranged generators that may be treated as infinite iterators.

```
> 100 random pop $1 pop $1 _ $list
[61 23]

> 100 random 3 nlist
[18 29 63]
```

### Threads
Starting a new thread copies program, stack and environment to a separate structure to minimize locking; sets the program counter after the last instruction, and calls the specified target. The target is only a starting point, threads are free to go wherever they want; a thread is finished once the program counter passes the last instruction. Anything remaining on the thread stack is pushed on the calling stack in ```join```

```
> 7 {35 +} thread join
42
```

### Macros
Besides a tiny core of fundamental functionality, the rest of Snabel is implemented as macros. A macro takes an incoming sequence of tokens and an outgoing sequence of VM-operations as parameters, both lists may be modified from within the macro.

```
add_macro(*this, "let:", [this](auto pos, auto &in, auto &out) {
  if (in.size() < 2) {
    ERROR(Snabel, fmt("Malformed binding on row %0, col %1",
	              pos.row, pos.col));
  } else {
    out.emplace_back(Backup(false));
    const str n(in.front().text);
    auto i(std::next(in.begin()));
	  
    for (; i != in.end(); i++) {
      if (i->text == ";") { break; }
    }

    compile(*this, pos.row, TokSeq(std::next(in.begin()), i), out);
    if (i != in.end()) { i++; }
    in.erase(in.begin(), i);
    out.emplace_back(Restore());
    out.emplace_back(Let(fmt("@%0", n)));
  }
});
```

### Running the code
Snabel is designed to eventually be compiled as a standalone library, but is currently being developed as part of [Snackis](https://github.com/andreas-gone-wild/snackis). The easiest way to trying out the examples is to get Snackis up and running, and execute 'script-new' to open the script view.

### License
Snabel is licensed under the GNU General Public License Version 3.

### Help
Please consider [helping](https://www.paypal.me/c4life) Snabel move forward, every contribution counts.<br/>