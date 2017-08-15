# Snabel
#### A statically typed scripting-language embedded in C++

![script example](images/script.png?raw=true)

### Postfix
Just like Yoda of Star Wars-fame, and yesterdays scientific calculators; as well as most printers in active use; yet unlike currently trending programming languages; Snabel expects arguments before operations.

```
> 7 42 + 10 %
9
I64
```

### Expressions
Snabel supports dividing expressions into parts using parentheses, each level starts with a fresh stack and the last result is pushed on the outer stack.

```
> (1 2 +) (2 2 *) +
7
I64
```

### Stacks
Literals, identifiers and results from function calls are pushed on the current fibers stack in order of appearance. Thanks to lexical scoping and named bindings, keeping the stack squeaky clean is less critical in Snabel.

```
> 1 2 3 stash
[1 2 3]

> 42 7 drop
42
I64

> 42 7 reset stash
[]

> 42 35 swap -
-7
```

### Functions
Snabel derives most of its type-checking powers from functions. Each named function represents a set of implementations, and each implementation may declare its parameter- and result types. Implementations are matched in reverse declared order when resolving function calls, to allow overriding existing functionality at any point. Prefixing the name of a function with ```&``` pushes it on the stack for later use.

Adding functions from C++ is as easy as this:

```
using namespace snabel;

static void add_i64(Scope &scp, FuncImp &fn, const Args &args) {
  Coro &cor(scp.coro);
  int64_t res(0);
  for (auto &a: args) { res += get<int64_t>(a); }
  push(cor, cor.exec.i64_type, res);
}

Exec exe;
add_func(exe, "+",
         {ArgType(exe.i64_type), ArgType(exe.i64_type)},
	 {ArgType(exe.i64_type)},
	 add_i64);
```

### Lambdas
Using braces instead of parentheses pushes a pointer to the compiled expression on the stack. Lambdas may be exited early by calling ```return``` and called recursively using ```recall```.

```
> {1 2 +}
n/a
Lambda

> {1 2 +} call
3
I64

> {
    1 2 + return
    14 *
  } call
3
I64

> 42
  {dec dup zero? &return when recall}
  call
0
```

### Bindings
Snabel supports named bindings using the ```let:```-macro. Bound names are prefixed with ```$```, lexically scoped and never change their value once bound in a specific scope. Semicolons may be used to separate bindings from surrounding code.

```
> let: fn {7 +}; 35 $fn call
42
I64
```

### Types
Snabel provides first class static, optionally parameterized types with inference.

```
> I64
I64
Type

> 42 I64 isa?
't
Bool

> 42 type
I64
Type
```

#### Lists
Snabel's lists are based on deques, which means fast inserts/removals in the front/back and decent random access-performance and memory layout. All list types are parameterized by element type. Lists allocate their memory on the heap and provide reference semantics.

```
> [1 2 3]
[1 2 3]
List<I64>

> Str list
[]
List<Str>

> 1 2 3 stash
[1 2 3]
List<I64>

> [35 7 + "foo"]
[42 "foo"]
List<Any>

> [1 2] 3 push reverse pop drop
[3 2]
List<I64>
```

### Labels
Snabel's control structures are based on the idea of jumping to offsets within the instruction stream. Beginning any name with ```@``` will create a label with the specified name at that point, while simply naming a label in scope will result in jumping there. Prefixing the name of a label in scope with ```&``` pushes it on the stack for later use.

```
> 1 2 3 +
5
I64

> 1 2 skip 42 @skip +
3
I64
```

### Conditions
```when``` accepts a condition and a callable target, the target is called if the condition is true. Possible targets are functions, lambdas and labels.

```
> 7 'f {35 +} when
7
I64

> 7 't {35 +} when
42
I64

> 7 35 't &+ when
42
I64
```

### Loops
The ```for```-loop accepts an iterable and a call target, and will call the target as long as the iterator returns more values. Possible iterables are numbers, which will call the target N times with N pushed on the stack; lists, which will call the target with successive items pushed on the stack; and strings, which will call the target with successive characters pushed on the stack.

```
> 0 7 &+ for
21
I64

> 0 [1 2 3 4 5 6] &+ for
21
I64

> "foo" &nop for stash \- join
"f-o-o"
Str
```

### Threads
Snabel was designed from the ground up to support multi-threading. Starting a new thread copies the entire program, stack and environment to a separate structure to minimize locking; sets the program counter after the last instruction, and calls the specified target. The target is only a starting point, threads are free to go wherever they want; a thread is finished once the program counter passes the last instruction. The last value on the thread stack is pushed on the calling stack when returning from ```join```.

```
> 7 {35 +} thread join
42
I64
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
Snabel is designed to eventually be compiled as a standalone library, but the code is currently being developed as part of [Snackis](https://github.com/andreas-gone-wild/snackis). The easiest way to trying out the examples is to first get Snackis up and running, and then execute 'script-new' to open the script view.

### License
Snabel is licensed under the GNU General Public License Version 3.

### Help
Please consider [helping](https://www.paypal.me/c4life) Snabel move forward, every contribution counts.<br/>