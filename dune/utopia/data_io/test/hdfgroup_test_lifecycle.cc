#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace Utopia::DataIO;

/// Asserts if two HDFGroup instances have the same public members
void assert_hdfgroups(HDFGroup& lhs, HDFGroup& rhs)
{
    assert(lhs.get_path() == rhs.get_path());
    assert(lhs.get_id() == rhs.get_id());
    assert(lhs.get_address() == rhs.get_address());
    assert(lhs.get_referencecounter() == rhs.get_referencecounter());
}

int main()
{
    HDFFile file("group_test_lifetime.h5", "w");

    HDFGroup first(file, "first");

    assert((*first.get_referencecounter())[first.get_address()] == 1);

    HDFGroup copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);
    assert_hdfgroups(first, copied_first);

    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);
    assert_hdfgroups(first, second);

    auto crosscheck(first); // this is needed  for checks
    crosscheck.close();     // but should  not take part in refcount anymore
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 3);

    assert_hdfgroups(crosscheck, moveassign_from_first);

    HDFGroup moveconst_second(std::move(second));

    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 3);
    assert_hdfgroups(crosscheck, moveconst_second);

    return 0;
}
