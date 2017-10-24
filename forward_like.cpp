
#include <iostream>
#include <vector>

#include <type_traits>

// given a facility that transfers cv-qualifiers from type to type
// clang-format off
// START_LIKE_DEF
template <typename From, typename To>
class like {
  template <bool Condition, template <typename> class Function, typename T>
  using apply_if = std::conditional_t<Condition, Function<T>, T>;
  using base = std::remove_cv_t<std::remove_reference_t<To>>;
  using base_from = std::remove_reference_t<From>;

  static constexpr bool rv = std::is_rvalue_reference_v<From>;
  static constexpr bool lv = std::is_lvalue_reference_v<From>;
  static constexpr bool c = std::is_const_v<base_from>;
  static constexpr bool v = std::is_volatile_v<base_from>;

public:
  using type = apply_if<lv, std::add_lvalue_reference_t,
               apply_if<rv, std::add_rvalue_reference_t,
               apply_if<c, std::add_const_t,
               apply_if<v, std::add_volatile_t,
               base>>>>;
};

template <typename From, typename To>
using like_t = typename like<From, To>::type;
// END_LIKE_DEF
// clang-format on

// ... we can define forward_like like so:
// START_FWD_LIKE_DEF
template <typename Like, typename T>
constexpr decltype(auto) forward_like(T&& t) noexcept
{
  // first, get `t` back into the value category it was passed in
  // then, forward it as if its value category was `Like`'s.
  // This prohibits rvalue -> lvalue conversions.
  return std::forward<like_t<Like, T>>(std::forward<T>(t));
}
// END_FWD_LIKE_DEF

template <typename T>
struct empty {
};

// TEST for forward_like
// START_FWD_LIKE_TEST_DEF
template <typename T, typename U>
using fwd_t = decltype(std::forward<T>(std::declval<U>()));
template <typename T, typename U>
using fwl_t = decltype(forward_like<T>(std::declval<U>()));
template <typename T, typename U>
auto inline static constexpr test_same_as_forward
    = std::is_same_v<fwd_t<T, U>, fwl_t<T, U>>;
// END_FWD_LIKE_TEST_DEF

int main()
{
  using std::declval;
  using std::forward;
  using std::is_same_v;
  using std::vector;

  // examples (and exhaustive tests)
  // START_LIKE_EXAMPLES
  static_assert(is_same_v<int, like_t<long, int const&&>>);
  static_assert(is_same_v<int&, like_t<long&, int const&&>>);
  static_assert(is_same_v<int&&, like_t<long&&, int const&&>>);
  static_assert(is_same_v<int const, like_t<long const, int>>);
  static_assert(is_same_v<int const&, like_t<long const&, int>>);
  static_assert(is_same_v<int const&&, like_t<long const&&, int>>);
  static_assert(is_same_v<int volatile const, like_t<long volatile const, int>>);
  static_assert(is_same_v<int volatile const&, like_t<long volatile const&, int>>);
  static_assert(is_same_v<int volatile const&&, like_t<long volatile const&&, int>>);
  static_assert(is_same_v<int volatile, like_t<long volatile, int>>);
  static_assert(is_same_v<int volatile&, like_t<long volatile&, int>>);
  static_assert(is_same_v<int volatile&&, like_t<long volatile&&, int>>);
  // END_LIKE_EXAMPLES

  // START_FWD_LIKE_TEST
  // std::forward<T> and std::forward_like<T> are the same
  static_assert(test_same_as_forward<int, int&>);
  static_assert(test_same_as_forward<int, int&&>);
  static_assert(test_same_as_forward<int&&, int&>);
  static_assert(test_same_as_forward<int&&, int&&>);
  static_assert(test_same_as_forward<int&, int&>);
#ifdef ENSURE_FAILS_TO_COMPILE_1 // fails.
  forward<int&>(int());
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_2
  forward_like<int&>(int());
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_3 // succeeds for some reason
  empty<fwd_t<int&, int&&>>{}; // check understanding
  static_assert(false, "TODO: Why does this succeed?");
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_4
  empty<fwl_t<int&, int&&>>{}; // check behavior
#endif

  static_assert(test_same_as_forward<int const, int&>);
  static_assert(test_same_as_forward<int const, int&&>);
  static_assert(test_same_as_forward<int const&&, int&>);
  static_assert(test_same_as_forward<int const&&, int&&>);
  static_assert(test_same_as_forward<int const&, int&>);
#ifdef ENSURE_FAILS_TO_COMPILE_5 // this is binding a rvalue-ref to const&...
  empty<fwd_t<int const&, int&&>>{}; // check understanding
  static_assert(false, "TODO: Why does this succeed?");
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_6
  empty<fwl_t<int const&, int&&>>{}; // check behavior
#endif

#ifdef ENSURE_FAILS_TO_COMPILE_7
  empty<fwd_t<int, int const&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_8
  empty<fwl_t<int, int const&>>{}; // check behavior
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_9
  empty<fwd_t<int, int const&&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_10
  empty<fwl_t<int, int const&&>>{}; // check behavior
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_11
  empty<fwd_t<int&&, int const&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_12
  empty<fwl_t<int&&, int const&>>{}; // check behavior
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_13
  empty<fwd_t<int&&, int const&&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_14
  empty<fwl_t<int&&, int const&&>>{}; // check behavior
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_15
  empty<fwd_t<int&, int const&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_16
  empty<fwl_t<int&, int const&>>{}; // check behavior
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_17
  empty<fwd_t<int&, int const&&>>{}; // check understanding
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_18
  empty<fwl_t<int&, int const&&>>{}; // check behavior
#endif

  static_assert(test_same_as_forward<int const, int const&>);
  static_assert(test_same_as_forward<int const, int const&&>);
  static_assert(test_same_as_forward<int const&&, int const&>);
  static_assert(test_same_as_forward<int const&&, int const&&>);
  static_assert(test_same_as_forward<int const&, int const&>);
#ifdef ENSURE_FAILS_TO_COMPILE_19
  empty<fwd_t<int const&, int const&&>>{}; // check understanding
  static_assert(false, "TODO: Why does this succeed?");
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_20
  empty<fwl_t<int const&, int const&&>>{}; // check behavior
#endif

  static_assert(is_same_v<int&, fwl_t<long&, int&>>);
  auto lvalue_vector = vector<int>{ 1, 2, 3 };
  forward_like<int&>(lvalue_vector);

  static_assert(is_same_v<int&&, fwl_t<long, int&>>);
  forward_like<int&&>(lvalue_vector);

#ifdef ENSURE_FAILS_TO_COMPILE_21
  empty<fwl_t<long&, int&&>>;
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_22
  empty<fwl_t<long&, int&&>>;
#endif
#ifdef ENSURE_FAILS_TO_COMPILE_23
  forward_like<int&>(vector<int>{ 1, 2, 3 });
#endif
  static_assert(is_same_v<int&&, fwl_t<long, int&&>>);
  forward_like<int&&>(vector<int>{ 1, 2, 3 });
  forward_like<int>(vector<int>{ 1, 2, 3 });
  // END_LIKE_TEST
}

// vim: textwidth=80
