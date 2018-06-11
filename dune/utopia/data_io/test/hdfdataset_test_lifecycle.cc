/**
 * @brief Tests the constructors of HDFDataset and the associated reference counting.
 *
 * @file hdfdataset_test_lifecycle.cc
 */
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

    assert(first.get_name() == copied_first.get_name());
    assert(first.get_id() == copied_first.get_id());
    assert(first.get_address() == copied_first.get_address());
    assert(first.get_referencecounter() == copied_first.get_referencecounter());
    assert(first.get_parent() == copied_first.get_parent());
    assert(first.get_rank() == copied_first.get_rank());

    for (std::size_t i = 0; i < first.get_rank(); ++i)
    {
        assert(first.get_extend()[i] == copied_first.get_extend()[i]);
        assert(first.get_capacity()[i] == copied_first.get_capacity()[i]);
    }

    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);

    assert(first.get_name() == second.get_name());
    assert(first.get_id() == second.get_id());
    assert(first.get_address() == second.get_address());
    assert(first.get_referencecounter() == second.get_referencecounter());
    assert(first.get_parent() == second.get_parent());
    assert(first.get_rank() == second.get_rank());

    for (std::size_t i = 0; i < first.get_rank(); ++i)
    {
        assert(first.get_extend()[i] == second.get_extend()[i]);
        assert(first.get_capacity()[i] == second.get_capacity()[i]);
    }

    auto crosscheck(first); // this is needed  for checks
    crosscheck.close();     // but let it not take part in refcount anymore
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 3);

    assert(crosscheck.get_name() == moveassign_from_first.get_name());
    assert(crosscheck.get_id() == moveassign_from_first.get_id());
    assert(crosscheck.get_address() == moveassign_from_first.get_address());
    assert(crosscheck.get_referencecounter() == moveassign_from_first.get_referencecounter());
    assert(crosscheck.get_parent() == moveassign_from_first.get_parent());
    assert(crosscheck.get_rank() == moveassign_from_first.get_rank());

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

    assert(crosscheck.get_name() == moveconst_second.get_name());
    assert(crosscheck.get_id() == moveconst_second.get_id());
    assert(crosscheck.get_address() == moveconst_second.get_address());
    assert(crosscheck.get_referencecounter() == moveconst_second.get_referencecounter());
    assert(crosscheck.get_parent() == moveconst_second.get_parent());
    assert(crosscheck.get_rank() == moveconst_second.get_rank());

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
