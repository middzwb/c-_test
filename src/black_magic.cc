#include <iostream>
#include <variant>
#include <cassert>
#include <string>
#include <tuple>

template <typename ...T>
auto sum(T... args) {
    return (args + ...);
}

template <typename T>
auto sum2(T t) {
    return t;
}
template <typename T0, typename ...Ts>
auto sum2(T0 t0, Ts... ts) {
    return (t0 + sum2(ts...));
}

template <typename ...T>
auto avg(T... args) {
    return (args + ...) / sizeof...(args);
}

template<size_t n, typename ...T>
constexpr std::variant<T...> _tuple_index(const std::tuple<T...>& tp, size_t i) {
    //if (n >= sizeof...(T)) {
    //    throw std::out_of_range("out_of_range");
    //}
    if (i == n) {
        return std::variant<T...>{std::in_place_index<n>, std::get<n>(tp)};
    }
    return _tuple_index<(n < sizeof...(T) - 1 ? n + 1 : 0)>(tp, i);
    //return _tuple_index<n+1>(tp, i);
}
template<typename ...T>
constexpr std::variant<T...>  tuple_index(const std::tuple<T...>& tp, size_t i) {
    //return std::get<_tuple_index<0>(tp, i).index()>(tp);
    return _tuple_index<0>(tp, i);
}
template <typename T0, typename ...T>
std::ostream& operator<<(std::ostream& o, const std::variant<T0, T...>& v) {
    std::visit([&](auto&& i) {
            o << i;
            }, v);
    return o;
}
template<typename ...T>
std::ostream& operator<<(std::ostream& o, const std::tuple<T...>& v) {
    o << '[';
    auto l = std::tuple_size_v<std::tuple<T...>>;
    for (auto i = 0; i < l - 1; ++i) {
        o << tuple_index(v, i) << " ";
    }
    o << tuple_index(v, l - 1);
    o << ']';
    return o;
}


void template_test() {
    using namespace std;
    std::cout << "template_test" << std::endl;
    assert(sum(1,2,3,4,5,6,7,8,9,10) == 55);
    assert(sum2(1,2,3,4,5,6,7,8,9,10) == 55);
    assert(avg(1,2,3,4,5,6,7,8,9) == 5);

    int* p = nullptr;
    delete p;

    std::tuple<int, char, std::string> tp{1, 's', "asd"};
    cout << tp << std::endl;
}
