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

    // assert ranked members
    for (std::size_t i = 0; i < lhs.get_rank(); ++i)
    {
        std::cerr << lhs.get_rank() << ","
                  << lhs.get_extend()[i] << ","
                  << rhs.get_extend()[i] << std::endl;
        assert(lhs.get_extend()[i] == rhs.get_extend()[i]);
        assert(lhs.get_capacity()[i] == rhs.get_capacity()[i]);
    }
}

int main()
{
    HDFFile file("dataset_test_lifetime.h5", "w");

    HDFGroup lifecyclegroup(*file.get_basegroup(), "livecycletest");
    std::vector<int> data(100, 42);

    HDFDataset first(lifecyclegroup, "first");

    first.write(data.begin(), data.end(), [](auto& value) { return value; });

    assert((*first.get_referencecounter())[first.get_address()] == 1);

    // copy constructor
    HDFDataset copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);
    assert_hdfdatasets(first, copied_first);

    // copy assignment
    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);
    assert_hdfdatasets(first, second);

    // move assignment
    auto crosscheck(first); // this is needed  for checks
    crosscheck.close();     // but let it not take part in refcount anymore
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 3);
    assert_hdfdatasets(crosscheck, moveassign_from_first);

    // move constructor
    HDFDataset moveconst_second(std::move(second));
    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 3);
    assert_hdfdatasets(crosscheck, moveconst_second);

    return 0;
}
