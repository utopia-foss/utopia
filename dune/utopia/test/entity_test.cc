//#include <random>
#include <dune/utopia/entity.hh>
#include <dune/utopia/state.hh>
#include <dune/utopia/tags.hh>
#include <iostream>
//#include <dune/common/exceptions.hh>

//#include "entity_test.hh"
//#include <cassert>

/** \param e Entity to test
 *  \param s State
 *  \param s_n New State
 *  \param t Traits
 *  \param t_n New Traits
 *  \param tag Tag
 */
/*template<typename Entity, typename State, typename Traits>
void assert_entity_members (Entity& e, const State& s, const State& s_n, const Traits& t, const Traits& t_n, const int tag)
{
    assert(e.state()==s);
    assert(e.new_state()==s_n);
    assert(e.traits()==t);
    assert(e.new_traits()==t_n);
    assert(e.tag()==tag);
}*/

/// Choose random states and traits. Check member access and update functions.
int main(int argc, char **argv)
{
    try{

        StateContainer<double, false> sc1(0.1);
        Utopia::DefaultTag tag(true);
        Utopia::Entity test_entity(sc1, tag, 0);
/*        Dune::MPIHelper::instance(argc,argv);

        using State = int;
        using Traits = double;

        /// get random values for states and traits
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<State> dist_state(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
        std::uniform_real_distribution<Traits> dist_traits(std::numeric_limits<Traits>::min(),std::numeric_limits<Traits>::max());
        const State state = dist_state(gen);
        const State state_1 = dist_state(gen);
        const State state_2 = dist_state(gen);
        const Traits traits = dist_traits(gen);
        const Traits traits_1 = dist_traits(gen);
        const Traits traits_2 = dist_traits(gen);
        const int tag = 1;

        /// test initialization
        Utopia::Entity<State,Traits> e1(state,traits,tag);
        assert_entity_members(e1,state,state,traits,traits,tag);

        /// test accessing state and traits cache
        e1.new_state() = state_1;
        e1.new_traits() = traits_1;
        assert_entity_members(e1,state,state_1,traits,traits_1,tag);

        /// test general update
        e1.update();
        assert_entity_members(e1,state_1,state_1,traits_1,traits_1,tag);

        /// test separate updates
        e1.new_state() = state_2;
        e1.update_state();
        assert_entity_members(e1,state_2,state_2,traits_1,traits_1,tag);

        e1.new_traits() = traits_2;
        e1.update_traits();
        assert_entity_members(e1,state_2,state_2,traits_2,traits_2,tag);
*/
        return 0;
    }
    //catch(Dune::Exception c){
    //    std::cerr << c << std::endl;
    //    return 1;
    //}
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
