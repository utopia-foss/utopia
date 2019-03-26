#include <array>
#include <cassert>
#include <iostream>
#include <sstream>
#include <tuple>
#include <utopia/core/compiletime_algos.hh>
#include <vector>

using namespace Utopia::Utils;
int main()
{
    // make some tuples
    std::tuple<int, double, std::string> s{42, 3.14, "a"};
    std::string x = "hello";
    std::array<double, 3> arr{{4.5, 5.5, 6.5}};

    // visit

    // expected result
    auto expected = std::make_tuple(std::make_tuple(42, x, 4.5),
                                    std::make_tuple(3.14, x, 5.5),
                                    std::make_tuple("a", x, 6.5));

    // check that the visitor visits each index in the right way
    visit(
        [](auto&& s_element, auto&& x_copy, auto&& arr_element, auto&& expect_element) {
            assert(std::make_tuple(s_element, x_copy, arr_element) == expect_element);
        },
        s, x, arr, expected);

    // reduce
    std::array<std::string, 3> expected_result{
        {"42_hello_4.50", "3.14_hello_5.50", "a_hello_6.50"}};

    auto result = reduce(
        // convert the arguments to a string and return them
        [](auto&& s_element, auto&& x_copy, auto&& arr_element) {
            std::ostringstream out;
            out.precision(2);
            out << std::fixed << s_element << "_" << x_copy << "_" << arr_element;
            return out.str();
        },
        s, x, arr);

    assert(std::get<0>(result) == std::get<0>(expected_result));
    assert(std::get<1>(result) == std::get<1>(expected_result));
    assert(std::get<2>(result) == std::get<2>(expected_result));

    // for_each
    for_each(result, [](auto&& str) { str += "_utopia"; });

    assert("42_hello_4.50_utopia" == std::get<0>(result));
    assert("3.14_hello_5.50_utopia" == std::get<1>(result));
    assert("a_hello_6.50_utopia" == std::get<2>(result));

    // transform
    result = transform(result, [](auto&& str) { return str + "_is_cool!"; });

    assert("42_hello_4.50_utopia_is_cool!" == std::get<0>(result));
    assert("3.14_hello_5.50_utopia_is_cool!" == std::get<1>(result));
    assert("a_hello_6.50_utopia_is_cool!" == std::get<2>(result));

    return 0;
}
