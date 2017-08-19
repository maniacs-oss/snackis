# Snabel
#### a fresh take on Forth

![script example](images/script.png?raw=true)

### Postfix
Like Yoda of Star Wars-fame, and yesterdays scientific calculators; as well as most printers in active use; yet unlike currently trending programming languages; Snabel expects arguments before operations.

```
> 7 42 + 10 %
9
I64!
```

### Streaming
Another difference that's important to be aware of is that Snabel views code as a stream, rather than a graph of operations. As long as the final sequence makes sense, Snabel mostly doesn't care how it got there. Notable exceptions are contents of strings, type parameter list and lambdas, where Snabel needs to see the final sequence to compile the code.

```
> bar label: foo; 35 +) baz label: bar; (7 foo label: baz
42
I64!

> bar label: foo; 35 +} baz label: bar; {7 foo label: baz
SnabelError: Missing lambda
```

### The Stack
Values and results from function calls are pushed on the current stack in order of appearance. Thanks to lexical scoping and named bindings, keeping the stack squeaky clean is less critical in Snabel. ```stash``` collects all values on the stack in a list and pushes it on the stack. ```$1```-```$9``` swaps in values, starting from the end; while ```$0``` duplicates the last value. ```_``` drops the last value and ```reset``` clears the entire stack.

```
> 1 2 3 stash
[1 2 3]
List<I64>!

> 1 2 $0 stash
[1 2 2]
List<I64>!

> 42 7 _ stash
[42]
I64!

> 42 7 reset stash
[]
List<Any>!

> 42 35 $1 -
-7
I64!
```

### Expressions
Parentheses may be used to divide expressions into separate parts, each level starts with a fresh stack and the last value is pushed on the outer stack on exit.

```
> (1 2 +) (2 2 *) +
7
I64!
```

### Lambdas
Wrapping code in braces pushes a pointer to the compiled expression on the stack. Lambdas may be exited early by calling ```return```, everything on the stack at the point of return is pushed on the outer stack. ```recall``` may be used to call the current lambda recursively.

```
> {1 2 +}
n/a
Lambda!

> {1 2 +} call
3
I64!

> {
    1 2 + return
    14 *
  } call
3
I64!

> 42
  {dec $0 zero? &return when recall 2 +}
  call
82
I64!
```

### Coroutines
Calling ```yield``` from within a lambda logs the current position, stack and environment before returning; execution will continue from the yielding position with restored stack and environment on next call from the same scope.

```
> let: foo {7 yield 28 +}
  $foo call $foo call +
42
I64!

> let: foo {let: bar 35; 7 yield $bar +}
  $foo call $foo call
42
I64!

> func: foo {[7 35] &yield for reset &+}
  foo foo foo call
42
I64!
```

### Equality
Two kinds of equality are supported, shallow and deep. Each use a separate operator, ```=``` for shallow comparisons and ```==``` for deep.

```
> [3 4 35] [3 4 35] =
#f
Bool!

> [3 4 35] [3 4 35] ==
#t
Bool!
```

### Bindings
The ```let:```-macro may be used to introduce named bindings. Bound names are prefixed with ```$```, lexically scoped and not allowed to change their value once bound in a specific scope. The bound expression is evaluated in a copy of the calling stack, which allows feeding values into a let statement but avoids unintended stack effects. Semicolons may be used to separate bindings from surrounding code.

```
> let: fn {7 +}; 35 $fn call
42
I64!
```

### Types
Types are first class, optionally parameterized and inferred.

```
> I64
I64
Type!

> 42 I64 is?
#t
Bool!

> 42 type
I64
Type!

> 42 Str!
Check failed, expected Str!
42 I64!
```

#### Rationals
Snabel provides exact arithmetics using rational numbers. Integers are promoted to rationals automatically as needed, while ```trunc``` may be used to convert rationals to integers.

```
> 1 3 /
1/3
Rat!

> 10 3 / trunc
3
I64!

> 1 1 /
  10 {_ 3 /} for
  10 {_ 3 *} for
1/1
Rat!
```

#### Lists
Lists are based on deques, which means fast inserts/removals in the front/back and decent random access-performance and memory layout. All list types are parameterized by element type. Lists allocate their memory on the heap and provide reference semantics.

```
> [1 2 3]
[1 2 3]
List<I64>!

> Str list
[]
List<Str>!

> 1 2 3 stash
[1 2 3]
List<I64>!

> [35 7 + 'foo']
[42 'foo']
List<Any>!

> [1 2] 3 push reverse pop _
[3 2]
List<I64>!
```

#### Pairs
Pairs have first class support and all iterables support zipping/unzipping. Pairs of values are created using ```.``` while ```zip``` operates on iterables, both values and iterables support ```unzip```.

```
> 'foo' 42.
'foo' 42.
Pair<Str I64>!

> ['foo' 'bar'] 7 list zip list
['foo' 0. 'bar' 1.]
List<Pair<Str I64>>!

> ['foo' 0. 'bar' 1.] unzip list $1 list stash
[[0 1] ['foo' 'bar']]
List<List<Any>>!
```

### Labels
The ```label:```-macro will create a label with the specified name at that point, while simply naming a label in scope will result in jumping there. Prefixing the name of a label in scope with ```&``` pushes it on the stack for later use.

```
> 1 2 3 +
5
I64!

> 1 2 skip 42 label: skip; +
3
I64!
```

### Conditions
```when``` accepts a condition and a callable target, the target is called if the condition is true. Possible targets are functions, lambdas and labels.

```
> 7 #f {35 +} when
7
I64!

> 7 #t {35 +} when
42
I64!

> 7 35 #t &+ when
42
I64!
```

### Iterators
Iteration is currently supported for numbers, which will return 0..N; lists, which will return successive items; and strings, which will return successive characters. And last, but not least; iterators themselves.

```
> 7 \, join
'0,1,2,3,4,5,6'
Str!

> let: foo 'bar' iter; $foo list
[\b \a \r]
List<Char>!
```

#### Loops
The ```for```-loop accepts an iterable and a call target, and will call the target with the last value pushed on the stack as long as the iterator returns values.

```
> 0 7 &+ for
21
I64!

> 0 [1 2 3 4 5 6] &+ for
21
I64!

> 'foo' &nop for stash \- join
'f-o-o'
Str!
```

### Functions
Each function name represents a set of implementations, and each implementation may declare its parameter- and result types. Implementations are matched in reverse declared order when resolving function calls, to allow overriding existing functionality at any point. Prefixing the name of a function with ```&``` pushes it on the stack for later use.

```
> func: foo {35 +}; 7 foo
42
I64!

> func: foo {35 +}
  let: bar &foo
  7 $bar call
42
I64!
```

#### C++
Adding functions from C++ is as easy as this:

```
using namespace snabel;

static void add_i64_imp(Scope &scp, const Args &args) {
  auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
  push(scp.coro, scp.exec.i64_type, x+y);
}

Exec exe;
add_func(exe, "+",
         {ArgType(exe.i64_type), ArgType(exe.i64_type)},
	 {ArgType(exe.i64_type)},
	 add_i64);
```

### Threads
Starting a new thread copies the entire program, stack and environment to a separate structure to minimize locking; sets the program counter after the last instruction, and calls the specified target. The target is only a starting point, threads are free to go wherever they want; a thread is finished once the program counter passes the last instruction. Anything remaining on the thread stack is pushed on the calling stack in ```join```

```
> 7 {35 +} thread join
42
I64!
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
    out.emplace_back(Let(fmt("$%0", n)));
  }
});
```

### Running the code
Snabel is designed to eventually be compiled as a standalone library, but is currently being developed as part of [Snackis](https://github.com/andreas-gone-wild/snackis). The easiest way to trying out the examples is to get Snackis up and running, and execute 'script-new' to open the script view.

### License
Snabel is licensed under the GNU General Public License Version 3.

### Help
Please consider [helping](https://www.paypal.me/c4life) Snabel move forward, every contribution counts.<br/>