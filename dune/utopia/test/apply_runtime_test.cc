#include <dune/utopia/utopia.hh>
#include <cassert>
int main(int argc, char* argv[])
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);

        const int grid_size = std::stoi(argv[1]);

        //build the ingredients for a manager
        auto grid = Utopia::Setup::create_grid(grid_size);
        auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);

        // build a manager - structured, periodic
        auto m_sync = Utopia::Setup::create_manager_cells<true, true>(
            grid, cells);

        // define rules calling by value and by reference
        auto empty_rule_copy = [](const auto cell, const auto manager){
            return cell->state();
        };
        auto empty_rule_ref = [](const auto cell, const auto& manager){
            return cell->state();
        };

        // time rule applications
        Dune::Timer t;
        Utopia::apply_rule(empty_rule_copy, m_sync.cells(), m_sync);
        std::cout << t.elapsed() << std::endl;
    
        t.reset();
        Utopia::apply_rule_ref(empty_rule_ref, m_sync.cells(), m_sync);
        std::cout << t.elapsed() << std::endl;

        return 0;
    }

    catch (...) {
        std::cout << "Exception thrown!" << std::endl;
        return 1;
    }
}