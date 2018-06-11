/**
 * @brief     This file implements tests for the lifecycle of HDFGroup, i.e. if the
    constructors do what they are supposed to, and if the reference counting
    system works as it should.

    This is done by checking if an HDFGroup is correctly instantiated,
    copied and moved and if the reference counter is incremented  or
    decremented respectively
 *
 * @file hdfgroup_test_lifecycle.cc
 * @author Harald Mack
 * @date 2018-06-11
 */

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
    // make file and group to use for copying moving etc.
    HDFFile file("group_test_lifetime.h5", "w");

    HDFGroup first(file, "first");

    // check if refcount of 'first' is one
    assert((*first.get_referencecounter())[first.get_address()] == 1);

    // check if copying works
    HDFGroup copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);

    assert(first.get_path() == copied_first.get_path());
    assert(first.get_id() == copied_first.get_id());
    assert(first.get_address() == copied_first.get_address());
    assert(first.get_referencecounter() == copied_first.get_referencecounter());

    // copy assignment
    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);

    assert(first.get_path() == second.get_path());
    assert(first.get_id() == second.get_id());
    assert(first.get_address() == second.get_address());
    assert(first.get_referencecounter() == second.get_referencecounter());

    // this crosscheck is needed because 'move' changes the object from which
    // something is moved, so we need a copy of it which stays unchanged
    // in order to check correct move.
    auto crosscheck(first); // this is needed  for checks
    crosscheck.close();     // but should  not take part in refcount anymore

    // check move assignment
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 3);

    assert(crosscheck.get_path() == moveassign_from_first.get_path());
    assert(crosscheck.get_id() == moveassign_from_first.get_id());
    assert(crosscheck.get_address() == moveassign_from_first.get_address());
    assert(crosscheck.get_referencecounter() == moveassign_from_first.get_referencecounter());

    // check move consturction
    HDFGroup moveconst_second(std::move(second));

    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 3);

    assert(crosscheck.get_path() == moveconst_second.get_path());
    assert(crosscheck.get_id() == moveconst_second.get_id());
    assert(crosscheck.get_address() == moveconst_second.get_address());
    assert(crosscheck.get_referencecounter() == moveconst_second.get_referencecounter());

    return 0;
}
