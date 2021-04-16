#include <iostream>
#include <cassert>

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

void template_test() {
    std::cout << "template_test" << std::endl;
    assert(sum(1,2,3,4,5,6,7,8,9,10) == 55);
    assert(sum2(1,2,3,4,5,6,7,8,9,10) == 55);
    assert(avg(1,2,3,4,5,6,7,8,9) == 5);
}
