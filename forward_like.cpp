
#include <iostream>
#include <type_traits>
#include <vector>


// given a facility that transfers cv-qualifiers from type to type
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
                 apply_if<v, std::add_volatile_t,
                 apply_if<c, std::add_const_t,
                 base>>>>;
};

template <typename From, typename To>
using like_t = typename like<From, To>::type;

// examples (and tests)
static_assert(std::is_same_v<int, like_t<long, int const&&>>);
static_assert(std::is_same_v<int&, like_t<long&, int const&&>>);
static_assert(std::is_same_v<int&&, like_t<long&&, int const&&>>);
static_assert(std::is_same_v<int const, like_t<long const, int>>);
static_assert(std::is_same_v<int const&, like_t<long const&, int>>);
static_assert(std::is_same_v<int const&&, like_t<long const&&, int>>);
static_assert(std::is_same_v<int volatile const, like_t<long volatile const, int>>);
static_assert(std::is_same_v<int volatile const&, like_t<long volatile const&, int>>);
static_assert(std::is_same_v<int volatile const&&, like_t<long volatile const&&, int>>);
static_assert(std::is_same_v<int volatile, like_t<long volatile, int>>);
static_assert(std::is_same_v<int volatile&, like_t<long volatile&, int>>);
static_assert(std::is_same_v<int volatile&&, like_t<long volatile&&, int>>);

// ... we can define forward_like like so:
template <typename Like, typename T>
constexpr decltype(auto) forward_like(T& t) noexcept
{
    return static_cast<like_t<Like&&, T>>(t);
}

template <typename Like, typename T,
    typename = std::enable_if_t<!(std::is_lvalue_reference_v<Like&&> && std::is_rvalue_reference_v<T&&>)>>
constexpr decltype(auto) forward_like(T&& t) noexcept
{
    static_assert(!(std::is_lvalue_reference_v<Like&&> && std::is_rvalue_reference_v<T&&>),
        "Trying to convert a rvalue to a lvalue is forbidden.");
    return static_cast<like_t<Like&&, T>>(t);
}

// TEST for forward_like
int main()
{
    {
        static_assert(std::is_same_v<int&, decltype(forward_like<long&>(std::declval<int&>()))>);
        auto x = std::vector<int>{ 1, 2, 3 };
        forward_like<int&>(x);
    }
    {
        static_assert(std::is_same_v<int&&, decltype(forward_like<long>(std::declval<int&>()))>);
        auto x = std::vector<int>{ 1, 2, 3 };
        forward_like<int&&>(x);
    }
    { // does not compile
#ifdef COMPILE_FAILING
        static_assert(std::is_same_v<int&, decltype(forward_like<long&>(std::declval<int&&>()))>, "should not compile");
        forward_like<int&>(std::vector<int>{ 1, 2, 3 });
#endif
    }
    {
        static_assert(std::is_same_v<int&&, decltype(forward_like<long>(std::declval<int&&>()))>);
        forward_like<int&&>(std::vector<int>{ 1, 2, 3 });
    }
}

// vim: textwidth=80
