#include <cassert>
#include <vector>
#include <iostream>

#include <dune/utopia/core/state.hh>

/// Instantiate containers, check access and contents
int main(int, char **)
{
    try{
        StateContainer<double, false> sc1(0.1);
        assert(!sc1.is_sync());
        auto& state = sc1.state();
        state = 0.2;
        assert(sc1.state() = 0.2);

        std::vector<double> vec({0.1, 0.2});
        StateContainer<std::vector<double>, true> sc2(vec);
        assert(sc2.is_sync());
        auto& new_state = sc2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(sc2.state() == vec);
        sc2.update();
        assert(sc2.state()[1] == 0.3);

        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
