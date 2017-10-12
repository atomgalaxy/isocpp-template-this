<pre>
Document number: PxxxxR0
Date:            2017-10-03
Project:         Programming Language C++, Evolution Working Group
Reply-to:        Ben Deane <ben at elbeno dot com>              
                 Gašper Ažman <gasper dot azman at gmail dot com>
</pre>

Template `this`! (Divining the value-category of `*this`)
================

Introduction
------------

We propose a new mechanism for specifying the value category of an instance of a
class which is visible from inside a member function of that class.
In other words, a way to tell from within a member function whether one's `this`
points to an rvalue or an lvalue, and whether it is const or volatile.

Motivation
----------

The existing mechanism for this (adding a reference, rvalue-reference, const, or
volatile qualifier (_cv-ref qualifiers_) suffixes to a member function) suffers
from problems.

### It is verbose. 

A common task in writing a class is providing a "getter function" to access a
contained member. Authors that care about performance want to provide getter
functions that take advantage of move semantics in the case where the class
instance is an rvalue, and so end up writing several of these functions, which
only differ in the cv-ref qualifiers .


### There is no way to write a reference qualifier for a lambda expression.

When writing a lambda expression, it is impossible to know whether it is safe to
move from captured-by-copy members of the closure object. This impacts the
performance of lambda expressions. One important scenario is when using lambda
expressions as parts of a chain of asynchronous continuations that carry a value
through a computation. Without knowing the value category of a closure object
from inside its function call operator, the implementer has a dilemma: incur a
copy, or use a move, and impose a requirement that is unenforceable in code and
sets hidden traps for the unwary.


Design Considerations
---------------------
   
In addition to solving the existing problems, desirable properties of a solution
are:

* It should work the same way for member functions and lambda expressions.

* It should work like existing practice as much as possible, while adding as
  little extra syntax as possible.

* It should avoid adding extraneous syntax to function declarations. In a world
  where every function is fast becoming tagged with attributes, constexpr,
  noexcept specifications, and (this proposal notwithstanding) reference
  qualifiers, readable, noise-free function declarations are to be prized.

TBD: more here.


Proposed Solution
-----------------

We propose the ability to add an optional first parameter to any member function
of a class `T`, taking the form `T [const] [volatile] [&|&&] this`.

To facilitate use in generic lambda expressions, this may also be formulated as
`auto [const] [volatile] [&|&&] this`.

In all cases, the value-category of `this` inside the member function is exactly
what the existing parameter rules already imply.

This, in the cases where the type of `this` is not a template, maps to the
existing practice of putting cv-ref qualifiers (that really apply to `*this` in
current parlance) on the function itself, by writing them behind the parameter
list, as far as the function signature is concerned.

```cpp
// Within a class

struct Person {
  std::string name;
  
  // Getter:
  template <typename U>
  decltype(auto) GetName(U&& this) { 
    // U can only deduce to Person (const, volatile, &, &&) because of
    // visibility
    return std::forward<T>(this).name;
  }

  // This template will likely instantiate the following functions:
  // const std::string& GetName() const &;
  // std::string& GetName() &;
  // std::string&& GetName() &&;
};

void foo()
{
  string s = "Hello, world!";
  
  // Here, the closure object is an rvalue:
  // the captured s will be correctly moved
  bar([s] (auto&& this) { 
    return std::forward_like<decltype(this)>(s); // separate TODO proposal
  }
}
```

What does `this` in a parameter list mean?
------------------------------------------

The meaning of the different ways to pass `this` is the same as current general
parameter handling.

The entries of this table should be read as if they are inside

```cpp
class T { /* entry */ };
```

In other words, `T` is _not_ a template parameter.

| written as                        | C++17 signature             | comments   | 
| --------------------------------- | --------------------------- | ---------- | 
| `void f(T this)`                  | currently not available     | [value]    | 
| `void f(T& this)`                 | `void f()&`                 |            | 
| `void f(T&& this)`                | `void f()&&`                |            | 
| `void f(T const this)`            | currently not available     | [value]    | 
| `void f(T const& this)`           | `void f() const&`           |            | 
| `void f(T const&& this)`          | `void f() const&&`          |            | 
| `void f(T volatile this)`         | currently not available     | [value]    | 
| `void f(T volatile& this)`        | `void f() volatile&`        |            | 
| `void f(T volatile&& this)`       | `void f() volatile&&`       |            | 
| `void f(T const volatile this)`   | currently not available     | [value]    | 
| `void f(T const volatile& this)`  | `void f() const volatile&`  |            | 
| `void f(T const volatile&& this)` | `void f() const volatile&&` |            | 

*Notes:*

- *[value]*: whether passing by value should be allowed is debatable, but seems
  obviously desired for completeness and parity with inline friend functions.
- The interpretation of `this` in the method body differs, but only one
  definition for a given signature may be present, eg. one may define at most
  one of `void f()&`, or `void f(T& this)` or `void f()`, the first and last
  already being exclusive of one another.


### How does templated `this` work?

Using existing deduction rules for template parameters, which will deduce the
type of `this` to something in the above table.


### What does `this` mean in the body of a method?

It behaves exactly as a regular parameter declared in the same way.


### Constructors

No exceptions to above rules. If a particular constructor signature is not
allowed by the language, it continues to be disallowed. We can already access
already-initialized members in initialization lists, which means `this` is
already available, even though it hasn't been completely constructed yet.


### What about pass by value methods?

We think they are a logical extension of the mechanism, and would go a long way
towards making methods as powerful as inline friend functions, with the only
difference being the call syntax.

One implication of this is that the `this` parameter would be move-constructed
in the case where the object is an _rvalue_, allowing you to treat chained
builder methods that return a new object uniformly _without_ having to resort to
templates.

*Example:*

```cpp
class string_builder {
  std::string s;

  operator std::string (T this) {
    return std::move(s);
  }
  string_builder operator*(T this, int n) {
    assert(n > 0);
    
    s.reserve(s.size() * n);
    auto const size = s.size();
    for (auto i = 0; i < n; ++i) {
      s.append(s, 0, size);
    }
    return this;
  }
  string_builder bop(T this) {
    s.append("bop");
    return this;
  }
};

// this is optimally efficient as far as allocations go
std::string const x = (string_builder{{"asdf"}} * 5).bop();
```

Of course, implementing this example with templated `this` methods would have
been more efficient due to just having fewer objects, but we got rid of all
references in the program!


### `this` as a reference

This paper turns `this` into a reference on an opt-in basis, which is in line
with the existing guidance that never-null pointers should be references if at
all possible, and in this case, it is possible.

We believe there would be no confusion, as in all cases, the value category of
`this` is stated plain as day in the parameter list, which is on the very same
screen.

Teaching also becomes easier, as the meaning of what a 'const method' is becomes
more obvious to students.

One can always obtain the address of the object by taking the address of the
`this`.


### Unification with `inline friend` functions

This proposal also makes `this` far less special. In fact, it completely unifies
`inline friend` functions and class methods, with the differences being:

- the calling syntax (method vs free function)
- methods can be virtual

Basically, if the first parameter is called `this`, one can parse and
instantiate the declaration with exactly the same rules as an `inline friend`,
except with a calling convention for methods.


### `virtual`

We are not entirely sure how `virtual` and value calling conventions would work,
so perhaps we can disallow that use-case, if the pass-by-value signatures are
kept in.


### Teachability Implications

Using `auto&& this` follows existing patterns for dealing with forwarding
references.


Optionally adding "this" as the first parameter fits with many programmers'
mental model of the `this` pointer being the first parameter to member functions
"under the hood" and is comparable to usage in other languages, e.g.  Python and
Rust.

It also works as a more obvious way to teach about how `std::bind` and
`std::function` with a method pointer work by making the pointer explicit.


### ABI Implications for `std::function` and Related

If references and pointers do not have the same representation for methods, this
effectively says "for the purposes of `this`, they do.".

Implications for lambdas
------------------------

Generic lambdas, should they take an `auto&& this` parameter, work according to
existing rewriting rules: the `auto&& this` is turned into a "universal
reference" and deduced as if it were inside a
`template <typename T> auto operator()(T&& this) -> <return type> {...}`.)

### Interplays with capturing `[this]` and `[*this]`

As `this` is passed as an explicit parameter, it shadows members of the closure.
That said, it's not possible to refer to the members of the closure using the
`this` pointer of the lambda, since the closure has no defined layout and the
members referenced may not even be inserted into the closure, if there even is
one.

TBD: does init-capture obviate all need for `*this`?

### Do we allow `this` in lambdas that decay to a function pointer? 

If the lambda would otherwise decay to a function pointer, `&this` shall have
the value of that function pointer.

### Does this allow recursion in lambdas?

Yup. You're allowed to call `this(...)`.


### Expressions allowed for `this` in lambdas:
```
this(...);      // call with appropriate signature
decltype(this); // evaluates to the type of the lambda with the appropriate
                // cv-ref qualifiers
&this;          // the address of either the closure object or function pointer
std::move(this) // you're allowed to move yourself into an algorithm...
/* ... and all other things you're allowed to do with the lambda itself. */
```

TBD: I don't think we should say that you can now refer to the closure object.
What you can do is deduce the value category of the closure object, and access
its members in a way appropriate to it.

TBD: Do we believe this?


Impact on the Standard
----------------------
   
TBD: A bunch of stuff in section 8.1.5 [expr.prim.lambda].
TBD: A bunch of stuff in that `this` can appear as the first method parameter.

Acknowledgments
---------------

Thanks to the following for their help and guidance:

- Louis Dionne
- Marshall Clow

References
----------

- P0018r3:[Capturing `*this` by Value](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0018r3.html)
- P0637r0:[Capture `*this` With Initializer](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0637r0.html)
- ...

We should probably look up the paper that added `*this` capture and why.
And any other salient papers.

