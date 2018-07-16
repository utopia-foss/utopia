#include "hdfutilities.hh"
#include <iostream>
#include <list>
#include <vector>
using namespace Utopia::DataIO;

int main()
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};
    std::vector<double> x{1.1, 2.2, 3.3};
    std::vector<double> y{4.4, 5.5, 6.6};
    auto a2 = a;
    auto x2 = x;
    std::cout << (a == a) << std::endl;
    std::cout << (a == b) << std::endl;
    std::cout << (a == std::move(a2)) << std::endl;
    std::cout << (b == std::vector<int>{4, 5, 6}) << std::endl;
    std::cout << (x == x) << std::endl;
    std::cout << (x == y) << std::endl;
    std::cout << (x == std::move(x2)) << std::endl;
    std::cout << (y == std::vector<double>{4.4, 5.5, 6.6}) << std::endl;
    std::cout << (std::vector<int>{1, 2, 3} == std::list<int>{1, 2, 3}) << std::endl;

    auto [rank, sizes] = container_properties() return 0;
}