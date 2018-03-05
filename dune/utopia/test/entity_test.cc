#include <dune/utopia/entity.hh>
#include <dune/utopia/state.hh>
#include <dune/utopia/tags.hh>
#include <iostream>
#include <cassert>

int main()
{
    try{
        // Test async entity with doubles and true tag
        StateContainer<double, false> sc1(0.1);
        Utopia::DefaultTag tag(true);
        Utopia::Entity test_entity(sc1, tag, 0);
        assert(!test_entity.is_sync());
        assert(test_entity.is_tagged  == true);
        auto& state = test_entity.state();
        state = 0.2;
        assert(test_entity.state() = 0.2);

        // Test sync entity with vector and false tag
    /*    std::vector<double> vec({0.1, 0.2});
        StateContainer<std::vector<double>, true> sc2(vec);
        tag.is_tagged = false;
        Utopia::Entity test_entity2(sc1, tag, 0);
        assert(test_entity2.is_tagged);
        assert(test_entity2.is_sync());
        auto& new_state = test_entity2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(test_entity2.state() == vec);
        test_entity2.update();
        assert(test_entity2.state()[1] == 0.3);*/

        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
