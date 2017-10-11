<pre>
Document number: PxxxxR0
Date:            2017-10-03
Project:         Programming Language C++, Evolution Working Group
Reply-to:        Ben Deane <ben at elbeno dot com>              
                 Gašper Ažman <gasper dot azman at gmail dot com>
</pre>

Divining the Value Category of `*this`
======================================

Introduction
------------

We propose a new mechanism for specifying the value category of an instance of a
class which can be divined from inside a member function of that class. In
short, a way to tell from within a member function whether one's `this` points
to an rvalue or an lvalue.

Motivation
----------

The existing mechanism for this (adding a reference qualifier suffix to a member
function) suffers from two problems:

### It is verbose. 

A common task in writing a class is providing a "getter function" to access a
contained member. Authors that care about performance want to provide getter
functions that take advantage of move semantics in the case where the class
instance is an rvalue, so end up duplicating, possibly triplicating these
functions.

### There is no way to write a reference qualifier for a lambda expression.

When writing a lambda expression, it is impossible to know whether it is safe to
move from captured-by-copy members of the closure object. This impacts the
performance of lambda expressions. One important scenario is when using lambda
expressions as parts of a chain of asynchronous continuations that carry a value
through a computation. Without knowing the value category of a closure object
from inside its function call operator, the implementer has a dilemma: incur a
copy, or use a move, and impose a requirement that is unenforceable in code and
sets hidden traps for the unwary.

Proposed Solution
-----------------

We propose the ability to add an optional first parameter to any member function
of a class `T`, taking the form `T&& this`. To facilitate use in lambda
expressions, this may also be formulated as `auto&& this`.

When this parameter is present, using `*this` inside the member function or
lambda expression would have the same effect as using a function argument passed
by forwarding reference, as seen in the following examples:

```cpp
// Within a class

struct Person {
  std::string name;
  
  // Getter - *this is deduced to be either:
  // const Person&, Person& or Person&&
  // and normal reference collapsing applies
  decltype(auto) GetName(Person&& this) {
    return std::forward<decltype(*this)>(*this).name;
  }

  // The following functions are replaced:
  // const std::string& GetName() const &;
  // std::string& GetName() &;
  // std::string&& GetName() &&;
};

// As a lambda expression

template <typename F>
void bar(F&& f) {
  // do something with std::forward<F>(f)
  // ...
}

void foo()
{
  string s = "Hello, world!";
  
  // Here, the closure object is an rvalue:
  // the captured s will be correctly moved
  bar([s] (auto&& this) { 
    return std::forward<decltype(*this)>(*this).s;
  }
}
```

From these examples it follows that a definition of a member function with `T&&
this` as its first parameter may be likened to declaring a free function
template that is a friend of `T` and that is constrained to take a forwarding
reference to `T` as its first argument. Usage of `*this` within the function is
very similar to existing practice dealing with arguments that are forwarding
references.

It is also clear that this opens the ability to refer to a closure object from
inside a lambda expression: something that until now has been impossible. We
believe this to be important for the performance reasons given. However, this is
perhaps undesirable in general and has the potential for poor interaction with
by-reference captures and existing `this` or `*this` capture. For this reason we
propose allowing using `*this` only to refer to by-copy captures inside a lambda
expression body.

We believe that existing `this` and `*this` capture is entirely obviated by init
capture and thus need not be used; any new code using `auto&& this` to access
the closure object has no need of `this` or `*this` capture.

TBD: Do we believe this?

## Design Decisions
   
In addition to solving the existing problems, desirable properties of a solution
are:

* It should work the same way for member functions and lambda expressions.

* It should work like existing practice as much as possible, while adding as
  little extra syntax as possible. Using `auto&& this` follows existing patterns
  for dealing with forwarding references. Optionally adding "this" as the first
  parameter fits with many programmers' mental model of the `this` pointer being
  the first parameter to member functions "under the hood" and is comparable to
  usage in other languages, e.g. Python.

* It should avoid adding extraneous syntax to function declarations. In a world
  where every function is fast becoming tagged with attributes, constexpr,
  noexcept specifications, and (this proposal notwithstanding) reference
  qualifiers, readable, noise-free function declarations are to be prized.

TBD: more here.

## Impact on the Standard
   
TBD: A bunch of stuff in section 8.1.5 [expr.prim.lambda].

## Acknowledgments

Thanks to the following for their help and guidance:

Louis Dionne

## References

We should probably look up the paper that added `*this` capture and why. And any
other salient papers.

