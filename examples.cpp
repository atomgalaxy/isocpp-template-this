#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// START_CLOSURE_EXAMPLE_DEFINITIONS
auto const text = std::string_view{ "Mired before cave STOP "
                                    "Only flesh wounds so far STOP "
                                    "Send holy hand grenade of Antioch STOP "
                                    "Love Knights of Ni STOP" };

// pretend this is some async handler - only the framework knows if it can
// consume the value or not
template <typename Source, typename... Sinks, typename Sink>
void tee(Source&& source, Sink&& sink, Sinks&&... other_sinks)
{
  if constexpr (sizeof...(other_sinks) > 0) {
    sink(source()); // a sink shall only ever be used once
    tee(std::forward<Source>(source), std::forward<Sinks>(other_sinks)...);
  } else { // last sink - we can inform the source it's going out of scope
    sink(std::forward<Source>(source)());
  }
}

// various handlers, some consume the value, some don't
auto print = [](auto&& message) { std::cout << message << std::endl; };

std::vector<std::string> precious;
auto save = [](auto&& message) {
  precious.emplace_back(std::forward<decltype(message)>(message));
};
// END_CLOSURE_EXAMPLE_DEFINITIONS

// START_CPP17
struct optimal_carrier {
  std::string message;
  std::string const& operator()() const & { return message; }
  std::string&& operator()() && { return std::move(message); }
};
// no way to have both && and const& call operators...
auto suboptimal_lambda = [message = std::string(text)]() -> auto const&
{
  return message;
};
// END_CPP17

#ifdef THIS_ARTICLE
// START_TEMPLATE_THIS
struct optimal_source {
  std::string message;
  template <typename Self>
  decltype(auto) operator()(Self&& this)
  {
    return std::forward<Self>(this).message;
  }
};
auto optimal_lambda
    = [message = std::string(text)](auto&& this) mutable -> decltype(auto)
{
  return std::forward_like<decltype(this)>(message);
};
// END_TEMPLATE_THIS
#endif

// START_CLOSURE_EXAMPLE_USAGE
void optimal_usage()
{
  auto carrier = optimal_carrier{ std::string(text) };
  tee(std::move(carrier), print, save); // zero copies
}

void best_lambda_usage()
{
  auto carrier = [message = std::string(text)]()->auto const& { return message; };
  tee(std::move(carrier), print, save); // incurs a copy
}
// END_CLOSURE_EXAMPLE_USAGE

#ifdef THIS_PAPER
// START_CAPTURE_STARTHIS
struct X {
  int operator()(int, int) const;
  void g(int n) const;

  auto f()
  {
    // capture *this by copy
    auto h = [*this](auto&& this, int n)
    {
      // decltype(this) is decltype(h) [const][&|&&], depending on which call
      // operator is invoked.
      g(n); // members of X are still accessible
      if (n % 2) {
        // calls this lambda recursively. Forward the cv-ref qualifiers to
        // sub-call.
        return std::forward<decltype(this)>(this)(n + 1);
      }
      return operator()(n, 2); // calls X::operator().
    };
    return h;
  }
};
// END_CAPTURE_STARTHIS
#endif

// START_MEMBER_REF_POINTER
struct Y {
  int f(int a, int b) const &;
};
static_assert(std::is_same_v<decltype(&Y::f), int (Y::*)(int, int) const &>);
// END_MEMBER_REF_POINTER

#ifdef THIS_PAPER
// START_MEMBER_VAL_POINTER
struct Z {
  int f(Z const& this, int a, int b);
  // same as `int f(int a, int b) const&;`
  int g(Z this, int a, int b);
};
// f is still the same as Y::f
static_assert(std::is_same_v<decltype(&Z::f), int (Z::*)(int, int) const &>);
// but would this alternate syntax make any sense?
static_assert(std::is_same_v<decltype(&Z::f), int (*)(Z::const&, int, int)>);
// It allows us to specify the syntax for Z as a pass-by-value member function
static_assert(std::is_same_v<decltype(&Z::g), int (*)(Z::, int, int)>);

// END_MEMBER_VAL_POINTER
#endif

int main()
{
  optimal_usage();
  best_lambda_usage();
}
