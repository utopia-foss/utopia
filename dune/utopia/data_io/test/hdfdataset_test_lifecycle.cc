#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace Utopia::DataIO;

/// Assert that two Dataset instances have the same public members
/** \tparam LHS Left hand side dataset type
 *  \tparam RHS Right hand side dataset type
 *  Template parameters are necessary because HDFDataset is a template
 */
template<class LHS, class RHS>
void assert_hdfdatasets(LHS& lhs, RHS& rhs)
{
    assert(lhs.get_name() == rhs.get_name());
    assert(lhs.get_id() == rhs.get_id());
    assert(lhs.get_address() == rhs.get_address());
    assert(lhs.get_referencecounter() == rhs.get_referencecounter());
    assert(lhs.get_parent() == rhs.get_parent());
    assert(lhs.get_rank() == rhs.get_rank());
}

int main()
{
    HDFFile file("dataset_test_lifetime.h5", "w");

    HDFGroup lifecyclegroup(*file.get_basegroup(), "livecycletest");
    std::vector<int> data(100, 42);

    HDFDataset first(lifecyclegroup, "first");

    first.write(data.begin(), data.end(), [](auto& value) { return value; });

    assert((*first.get_referencecounter())[first.get_address()] == 1);

    HDFDataset copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);

    assert_hdfdatasets(first, copied_first);

    for (std::size_t i = 0; i < first.get_rank(); ++i)
    {
        assert(first.get_extend()[i] == copied_first.get_extend()[i]);
        assert(first.get_capacity()[i] == copied_first.get_capacity()[i]);
    }

    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);

    assert_hdfdatasets(first, second);

    for (std::size_t i = 0; i < first.get_rank(); ++i)
    {
        assert(first.get_extend()[i] == second.get_extend()[i]);
        assert(first.get_capacity()[i] == second.get_capacity()[i]);
    }

    auto crosscheck(first); // this is needed  for checks
    crosscheck.close();     // but let it not take part in refcount anymore
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 3);
    assert_hdfdatasets(crosscheck, moveassign_from_first);

    for (std::size_t i = 0; i < crosscheck.get_rank(); ++i)
    {
        std::cerr << moveassign_from_first.get_rank() << ","
                  << moveassign_from_first.get_extend()[i] << ","
                  << crosscheck.get_extend()[i] << std::endl;
        assert(crosscheck.get_extend()[i] == moveassign_from_first.get_extend()[i]);
        assert(crosscheck.get_capacity()[i] == moveassign_from_first.get_capacity()[i]);
    }

    HDFDataset moveconst_second(std::move(second));

    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 3);
    assert_hdfdatasets(crosscheck, moveconst_second);

    for (std::size_t i = 0; i < crosscheck.get_rank(); ++i)
    {
        std::cerr << moveconst_second.get_rank() << ","
                  << moveconst_second.get_extend()[i] << ","
                  << crosscheck.get_extend()[i] << std::endl;
        assert(crosscheck.get_extend()[i] == moveconst_second.get_extend()[i]);
        assert(crosscheck.get_capacity()[i] == moveconst_second.get_capacity()[i]);
    }
    return 0;
}
