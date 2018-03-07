#include <dune/utopia/utopia.hh>
#include <iostream>
#include <vector>
#include <cassert>

int main()
{
    try{
        // Test async entity with doubles
        Utopia::Entity<double, false, Utopia::DefaultTag, int> test_entity(0.1, 0);
        assert(!test_entity.is_sync());
        auto& state = test_entity.state();
        state = 0.2;
        assert(test_entity.state() = 0.2);
        // Test members inherited from Tag
        assert(!test_entity.is_tagged);
        test_entity.is_tagged = true;
        assert(test_entity.is_tagged);
        // Test entity member variables
        assert(test_entity.id() == 0);

        // Test sync entity with vector 
        std::vector<double> vec({0.1, 0.2});
        Utopia::Entity<std::vector<double>, true, Utopia::DefaultTag, int> test_entity2(vec, 987654321);
        assert(test_entity2.id() == 987654321);
        assert(test_entity2.is_sync());
        auto& new_state = test_entity2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(test_entity2.state() == vec);
        test_entity2.update();
        assert(test_entity2.state()[1] == 0.3);

        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
