#include <dune/utopia/utopia.hh>

#include "dummy.hh"

int main ()
{
    try {
        std::vector<double> state(1E6, 0.0);
        Utopia::DummyModel model(state);

        assert(compare_containers(model.data(), state));

        model.iterate();
        state = std::vector<double>(1E6, 1.0);
        assert(compare_containers(model.data(), state));

        model.set_boundary_condition(std::vector<double>(1E6, 2.0));
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        state = std::vector<double>(1E6, 1.0);
        model.set_initial_condition(state);
        assert(compare_containers(model.data(), state));

        // check override of iterate function
        Utopia::DummyModelWithIterate model_it(state);
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        return 0;
    }
    catch (...) {
        return 1;
    }
}