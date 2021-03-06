![logo](images/logo.png?raw=true)

# Snabel
#### a decently typed Forth with a touch of Perl

```
#!/usr/local/bin/snabel

/*
  hgrm.sl

  Reads text from STDIN and writes N most frequent words longer than M
  ordered by frequency to STDOUT.                                

  Usage:
  cat *.txt | hgrm.sl N M
*/

let: min-wlen stoi64; _
let: max-len stoi64; _
let: tbl Str I64 table;

stdin read unopt
words unopt
{len @min-wlen gte?} filter
{$ downcase} map
{@tbl $1 1 &+1 upsert _} for
@tbl list $ {right $1 right lt?} sort
@max-len nlist {unzip '$1\t$0' say _ _} for
```

### Status
Snabel still has quite some way to go before claiming general purpose; it's evolution is still mostly driven by my own needs and interests. I have yet to do any serious comparisons, or optimization; but its more than fast enough for scripting.

### Dependencies
Snabel requires a ```C++1z```-capable compiler and standard library to build, and defaults to using clang with ```libc++```. This unfortunately still often means downloading and manually installing [clang](http://releases.llvm.org/download.html#4.0.0) to even run Snabel, but will improve over time. Snabel further depends on ```libpthread```, ```libsodium``` and ```libuuid```.

```
tar -xzf clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
cd clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04
sudo cp -R * /usr/local/
sudo ldconfig

sudo apt-get install cmake libsodium-dev uuid-dev
```

### Getting Started
If you're running ```Linux/64```, copy ```snabel``` from [/dist](https://github.com/andreas-gone-wild/snackis/tree/master/dist) to ```/usr/local/bin``` and run ```rlwrap snabel```; otherwise you'll have to build the executable yourself.

#### Building
Once dependencies are satisfied, executing the following commands will build Snabel:

```
git clone https://github.com/andreas-gone-wild/snackis.git
mkdir snackis/build
cd snackis/build
cmake ..
make snabel
```

### Running Code
Snabel supports two modes of operation; launching with arguments treats the first one as a filename to load with remaining arguments pushed on the stack, while launching without starts the REPL. ```load``` may be used to load files from within the REPL.

```
Snabel v0.7.15

Press Return on empty line to evaluate,
Ctrl-C exits.

S: 'foo.sl' load

nil
```

### Postfix
Like Yoda of Star Wars-fame, and yesterdays scientific calculators; as well as most printers in active use; yet unlike currently trending languages; Snabel expects arguments before operations.

```
S: 'Hello World!' say

Hello World!
nil

S: 7 35 +

42
```

### Inverted Control
Many of Snabel's features may look weird on first contact, but once you take a closer look you'll realize that it's the same old friends turned more or less inside out. One of the guiding principles of Snabel is to push as much control as possible to user code. So you get full stack access instead of having to specify everything upfront, iterators instead of callbacks, gradual typing instead of either or, deferred actions instead of try/finally. 

### Decently Scoped
While most languages tend to draw a circle around their core, inside which lexical scoping doesn't apply; usually functions, macros, types and similar; Snabel goes all in to apply lexical scoping wherever possible. Snabel also provides multiple levels of call stack control; ```break```, ```defer```, ```yield```, ```recall``` and ```return``` all support targeting outer scopes by specifying an index between 1 and 9.

```
S: {struct: Foo
      bar baz;} call

   Foo new

Error: Unknown identifier: Foo

S: {{42 return1} call 7 -} call

42
```

### The Stack
Values and results from function calls are pushed on the stack in order of appearance. Thanks to lexical scoping and named bindings, keeping the stack squeaky clean is less critical in Snabel. ```$dump``` collects all values on the stack in a list that is pushed instead. ```$1```-```$9``` swaps in values, starting from the end; while ```$``` duplicates the last value. ```_``` drops the last value and ```|``` clears the entire stack.

```
S: 1 2 3 $dump

[1 2 3]

S: 1 2 $ $dump

[1 2 2]

S: 42 7 _ $dump

[42]

S: 42 7 | $dump

[]

S: 42 35 $1 -

-7
```

### Expressions
Parentheses may be used to divide expressions into separate parts, each level copies the stack on entry and restores it with the result (if any) pushed on exit.

```
S: (1 2 +) (2 2 *) +

7

S: 1 (2 3 $dump)

[1 2 3]

S: 1 (|2 3 $dump) .

1 [2 3].
```

### Comments
Snabel supports both block- and single line comments in code.

```
S: /* This is
      a block
      comment */

S: // This is a single line comment
```

### Equality
Two kinds of equality are supported, shallow and deep. Each use a separate operator, ```=``` for shallow comparisons and ```==``` for deep.

```
S: [3 4 35] [3 4 35] =

false

S: [3 4 35] [3 4 35] ==

true
```

### Bindings
The ```let:```-macro may be used to introduce named bindings. Bound names are prefixed with ```@```, lexically scoped and may be shadowed at any time by another let. The bound expression is evaluated in a copy of the calling stack, which allows feeding values in but avoids unintended stack effects outside. Bindings require termination using ```;``` to separate them from surrounding code.

```
S: let: fn {7 +}; 35 @fn call

42
```

### Types
Types are first class, optionally parameterized and inferred. Primitive types like booleans, characters and integers use value semantics while composite types such as pairs, strings, lists and tables are accessed by reference.

```
S: I64

I64

S: 42 type

I64

S: [7 35] List<I64> is?

true
```

#### Strings
Strings are mutable collections of characters that provide reference semantics. Snabel makes a difference between byte strings and unicode strings. Unicode strings are delimited by double quotes, processed as UTF-8 when converting to/from bytes and stored internally as UTF-16. Byte strings are delimited by single quotes and automatically promoted to unicode as needed.

```
S: 'foo'

'foo'

S: 'foo' 'foo' =

false

S: 'foo' 'foo' ==

true

S: "foo"

"foo"

S: 'foo' ustr

"foo"

S: "foo" 'foo' =

true

S: 'ö' len

2

S: "ö" len

1

S: ['foo\r\n\r\nbar\r\n\r\nbaz' bytes]
   lines unopt \, join

'foo,bar,baz'
```

#### Characters
Character literals are supported for both unicode- and byte strings.

```
S: '' $ \f push $ \o push $ \o push

'foo'

S: "" $ \\f push $ \\o push $ \\o push

"foo"
```

#### Symbols
Symbols are unique strings that support efficient comparisons/ordering. Snabel uses symbols internally for all names. Any string may be interned as a symbol, but it makes most sense for smaller sets of short strings that are reused a lot.

```
S: #foo

#foo

S: #foo $ =

true

S: #foo 'foo' sym =

true

S: #foo str 'foo' =

true

S: #foo #bar =

false

S: #foo #bar gt?

true
```

#### Rational Numbers
Snabel supports exact arithmetics using rational numbers. Integers are promoted to rationals automatically as needed, while ```trunc``` may be used to convert rationals to integers.

```
S: 1 3 /

1/3

S: 10 3 / trunc

3

S: 1 1 /
   10 {_ 3 /} for
   10 {_ 3 *} for

1/1
```

#### Lists
Lists are based on deques, which means fast inserts/removals in the front/back and decent random access-performance and memory layout. All list types are parameterized by element type, allocate their memory from the heap and provide reference semantics.

```
S: [1 2 3]

[1 2 3]

S: Str list

[]

S: 1 2 3 $dump

[1 2 3]

S: [35 7 + 'foo']

[42 'foo']

S: [1 2] $ 3 push $ reverse $ pop _

[3 2]

S: [2 3 1] $ &lt? sort

[1 2 3]
```

#### Pairs
Pairs have first class support and all iterables support zipping/unzipping. Pairs of values are created using ```.``` while ```zip``` operates on iterables, both values and iterables support ```unzip```.

```
S: 'foo' 42.

'foo' 42.

S: 'foo' 42. right

42

S: 'foo' 42. unzip

'foo' 42

S: ['foo' 'bar'] 7 list zip list

['foo' 0. 'bar' 1.]

S: ['foo' 0. 'bar' 1.] unzip list $1 list $dump

[[0 1] ['foo' 'bar']]
```

#### Optional Values
Optional values are supported through the ```Opt<T>```-type. The empty value is called ```nil```.

```
S: 42 opt

Opt(42)

S: 7 opt {35 +} when

42

S: nil {42} unless

42

S: nil 42 or

42

S: nil 42 opt or

Opt(42)

S: 7 opt 42 opt or

Opt(7)

S: [7 opt nil 35 opt]

[Opt(7) nil Opt(35)]

S: [7 opt nil 35 opt] unopt list

[7 35]
```

#### Tables
Tables map keys to values, and may be created from any pair iterable. Iterating a table results in a pair iterator.
```
S: [7 'foo'. 35 'bar'.] table 35 get

Opt('bar')

S: let: acc Str I64 table;
   ['foo,\nbar.baz;\nfoo!' bytes] words
   unopt
   { @acc $1 1 &+1 upsert } for

['bar' 1. 'baz' 1. 'foo' 2.]
```

#### Structs
Structs may be used to create lexically scoped, composite data types. Constructors and typed field accessors are automatically created. All fields are expected to be initialized when read, reading uninitialized fields signals errors; use optional types for optional fields. Any number of super types may be listed after the struct name, fields with the same name share storage. Structs are iterable and produce a sequence of symbol/value-pairs. When redefining structs, fresh types are created each time; existing instances reference the previous type as do already compiled type literals.

```
S: struct: Foo
     a I64 b c;

   Foo new Foo is?

true

S: Foo new
   $ 7 set-a
   $ 14 set-b
   $ 'bar' set-c;
   
Foo(a 7. b 14. c 'bar'.)

S: Foo new $ 21 set-c list

[#c 21.]

S: struct: Bar Foo
     d List<Str>;

   Bar new Foo is?
   
true
```

#### Conversions
Snabel supports user-defined automatic promotions of function arguments. ```conv:``` takes a source type, a target type and an expression that is executed with the value pushed on promotion. Manual conversion is supported through ```conv``` which may be overloaded to define conversions that don't take part in automatic promotion. Snabel comes preloaded with promotions from integers to rationals, byte to unicode strings and strings to file paths.

```
S: struct: Foo
     a;

   struct: Bar
     b;

   Foo new Bar conv

nil

S: conv: Foo Bar
     Bar new $ 42 set-b opt;
   
   Foo new Bar conv

Opt(Bar(b 42.))

S: func: say-bar(b Bar)
     '@b' say

   Foo new
   $ 'foobar' set-a
   say-bar

'Foo(a 'foobar'.)'

S: func: conv(v I64 t Type<Str>)
     @v str opt;

   42 Str conv

Opt('42')
```

### Lambdas
Wrapping code in braces pushes a pointer on the stack. Lambdas may be exited early by calling ```return```, the final result (if any) is pushed on exit. ```recall``` may be used to call the current lambda recursively. Use ```&return```/```&recall``` to get a target that performs the specified action when called.

```
S: {1 2 +}

Lambda(_enter1:0)

S: {1 2 +} call

3

S: 42
   {-- $ z? &return when recall}
   call
   
0
```

#### Deferred Actions
Deferred actions are executed in reverse defined order on scope exit.

```
S: {1
    {10} defer
    {100} defer
    return
    1000} call $dump
   
[1 100 10]
```

### Conditions
```when``` takes a condition and a callable target, the target is called if the condition is true. ```unless``` is the opposite of ```when```. ```if``` takes a condition and two targets, one that's called if the condition is true and one thats called otherwise. ```select``` takes an iterator of conditions/target pairs, and calls the first target which condition returns true.

```
S: 7 true {35 +} when

42

S: 7 false {35 +} when

7

S: 7 true {35 +} unless

7

S: 7 false {35 +} unless

42

S: true 42 nil if

42

S: false 42 nil if

nil

S: func: guess-pos(w)
     [{@w 'ed' suffix?}
        #VBD.
      {@w 'es' suffix?}
        #VBZ.
      {@w 'ing' suffix?}
        #VBG.
      {@w 'ould' suffix?}
        #MD.
      {@w '\'s' suffix?}
        #NN$.
      {@w 's' suffix?}
        #NNS.
      {true}
        #NN.] select;

   'flower\'s' guess-pos 

#NN$
```

### Iterators
Many things in snabel are iterable, numbers, strings and lists to name a few. Snabel encourages composing iterables into data pipelines for efficient processing.

```
S: 7 \, join

'0,1,2,3,4,5,6'

S: [1 2 3] {7 *} map list

[7 14 27]

S: 'abcabcabc' {\a =} filter str

"aaa"
```

#### Loops
The ```for```-loop accepts an iterable and a target, and calls the target with the last value pushed on the stack as long as the iterator returns values; while ```loop``` calls the target until it exits by other means. ```break``` may be used to exit loops early, use ```&break``` to get a target that breaks when called.

```
S: 0 7 &+ for

21

S: 0 7 {$ 5 = {_ break} when +} for

10

S: 0 [1 2 3 4 5 6] &+ for

21

S: 'foo' &nop for $dump \- join

'f-o-o'

S: 0 {+1 $ 42 = &break when} loop

42
```

### Functions
Each function name represents a set of implementations that are matched in reverse declared order when resolving function calls. Prefixing the name of a function with ```&``` pushes it on the stack for later use. Functions support optional arguments with optional types, types may refer to other arguments and their type parameters using ```N[:N]```; and require termination using ```;``` to separate them from surrounding code.

```
S: func: foo 35 +; 7 foo

42

S: func: foo(x) @x 35 +; 7 foo

42

S: func: foo(x) @x 35 +;
   let: bar &foo
   7 @bar call

42

S: func: foo(x I64) 42; 'bar' foo

Error: Function not applicable: foo

S: func: foo(x List<Any> y 0:0)
     @x @y push;

   [7]
   $ 14 foo
   pop

14

S: func: foo(x Num) @x 7 +;
   func: foo(x I64) @x 14 +;
   35 foo

49

S: 35 foo<Num>

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
add_func(exe, "+", Func::Pure,
         {ArgType(exe.i64_type), ArgType(exe.i64_type)},
	 add_i64);
```

### Coroutines
Calling ```yield``` from a lambda logs the current position, stack and environment before returning; execution continues from the yielding position with restored stack and environment on next call. A coroutine context is returned on first ```yield```, calling the context resumes execution.  Use ```&yield``` to get a target that yields when called.

```
S: {yield 42} call

Coro(_enter1:1)

S: {yield 42} call call

42

S: func: foo |yield (7 yield 28 +);
   foo $ call $1 call +

42

S: let: acc Str list;
   func: do-ping()
     (|yield 3 {@acc 'ping' push yield1} for);
   func: do-pong()
     (|yield 3 {@acc 'pong' push yield1} for);
   [do-ping do-pong] run
   @acc

['ping' 'pong' 'ping' 'pong 'ping' 'pong']
```

### IO
Snabel provides non-blocking IO in the form of iterators. The provided target is called with each read chunk or written number of bytes pushed on the stack. Files come in two flavors, read-only and read-write.

```
S: ['foo' 'bar' 'baz'] &say for

foo
bar
baz
nil

S: 'snackis' rfile

RFile(11)

S: 'snackis' rfile read

Iter<Bin>

S: 0 'snackis' rfile read {{len +} when} for

2313864

S: 'tmp' rwfile

RWFile(12)

S: 'foo' bytes 'tmp' rwfile write

Iter<I64>

S: ['foo' bytes]
   'tmp' rwfile
   write 0 $1 &+ for
   
3

S: func: do-copy(in out) (
     let: q Bin list;
     let: wq @q fifo;
     let: r @in rfile read;
     let: w @wq @out rwfile write;
     |_yield
  
     @r {{
       @q $1 push
       @w &break _for} &_ if
       
       _yield1
     } for

     @q +? {@w &_ for} when);

   'in' 'out' do-copy run
nil
```

### Networking
Snabel supports basic asynchronous TCP/IP-networking, you may find more examples [here](https://github.com/andreas-gone-wild/snackis/tree/master/dist).

```
S: let: addr '127.0.0.1';
   let: port 31331;

   let: server tcp-socket
               @addr @port bind;

   let: sender tcp-socket
               @addr @port
               connect;

   func: do-connect() (
     |yield
     @client {&break &_yield1 if} for
   );

   func: do-accept() (
     |yield
     @server 0 accept {&break &_yield1 if} for
   );

   [do-connect do-accept] run

   let: in; _
   let: out; _

   ['hello world' bytes] @out write &nop _for
   @out close
   @in read unopt words unopt list
   @server close

['hello' 'world']
```

### Random Numbers
Random numbers are supported through ranged generators that may be treated as infinite iterators.

```
S: 100 random $ pop $1 pop $1 $dump

[61 23]

S: 100 random 3 nlist

[18 29 63]
```

### Threads
Starting a new thread copies program, stack and environment to a separate structure to minimize locking; sets the program counter after the last instruction, and calls the specified target. The target is only a starting point, threads are free to go wherever they want; a thread is finished once the program counter passes the last instruction. Anything remaining on the thread stack is pushed on the calling stack in ```join```

```
S: 7 {35 +} thread join

42
```

### Sandboxing
Functions defined via the host api may be tagged as safe or unsafe. Safe functions are not allowed to cause external effects; most of the functionality in Snabel's IO, network and thread vocabularies is tagged as unsafe. Calling ```safe``` increases the safety level for the current scope; there is no way of decreasing it short of closing the scope; and the level is inherited by sub scopes. Environment lookups are limited to the same safety level, stack access is only allowed within the same level, and executable definitions inherit the current level on compilation. 

```
S: {35 7 safe +} call
Error: Unsafe stack access

S: { let: foo 42;
     {safe '@foo;bar'} call
   } call

Error: Unknown identifier: @foo

S: { safe
     '{\'~/.bash_history\' rfile}' eval
   } call call

Error: Unsafe call not allowed: rfile
```

### Macros
A macro takes an incoming sequence of tokens and an outgoing sequence of VM-operations as parameters, both lists may be modified from within the macro.

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

### License
Snabel is licensed under the GNU General Public License Version 3.

### Help
Please consider [helping](https://www.paypal.me/c4life) Snabel move forward, every contribution counts.<br/>