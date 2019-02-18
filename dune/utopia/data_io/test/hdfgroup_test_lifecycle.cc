/**
 * @brief     This file implements tests for the lifecycle of HDFGroup, i.e. if the
    constructors do what they are supposed to, and if the reference counting
    system works as it should.

    This is done by checking if an HDFGroup is correctly instantiated,
    copied and moved and if the reference counter is incremented  or
    decremented respectively
 *
 * @file hdfgroup_test_lifecycle.cc
 */

#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <dune/common/parallel/mpihelper.hh>
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
    assert(lhs.get_referencecounter().get() == rhs.get_referencecounter().get());
}

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    // make file and group to use for copying moving etc.
    HDFFile file("group_test_lifetime.h5", "w");

    HDFGroup first(file, "first");

    // check if refcount of 'first' is one
    assert((*first.get_referencecounter())[first.get_address()] == 1);

    // check if copying works
    HDFGroup copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);
    assert_hdfgroups(first, copied_first);

    assert(first.get_path() == copied_first.get_path());
    assert(first.get_id() == copied_first.get_id());
    assert(first.get_address() == copied_first.get_address());
    assert(first.get_referencecounter().get() ==
           copied_first.get_referencecounter().get());

    // copy assignment
    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);
    assert_hdfgroups(first, second);

    assert(first.get_path() == second.get_path());
    assert(first.get_id() == second.get_id());
    assert(first.get_address() == second.get_address());
    assert(first.get_referencecounter() == second.get_referencecounter());

    // this crosscheck is needed because 'move' changes the object from which
    // something is moved, so we need a copy of it which stays unchanged
    // in order to check correct move.
    auto crosscheck(first); // this is needed  for checks -> refcount of first goes one up!
    assert_hdfgroups(crosscheck, first);

    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 4);
    assert_hdfgroups(crosscheck, moveassign_from_first);

    assert(crosscheck.get_path() == moveassign_from_first.get_path());
    assert(crosscheck.get_id() == moveassign_from_first.get_id());
    assert(crosscheck.get_address() == moveassign_from_first.get_address());
    assert(crosscheck.get_referencecounter() == moveassign_from_first.get_referencecounter());

    // check move consturction
    HDFGroup moveconst_second(std::move(second));
    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 4);
    assert_hdfgroups(crosscheck, moveconst_second);

    // check open method
    HDFGroup opened_group;
    while (check_validity(H5Iis_valid(moveconst_second.get_id()),
                          moveconst_second.get_path()))
    {
        moveconst_second.close();
    }
    assert(!check_validity(H5Iis_valid(moveconst_second.get_id()),
                           moveconst_second.get_path()));
    opened_group.open(*file.get_basegroup(), "first");
    assert(check_validity(H5Iis_valid(opened_group.get_id()), opened_group.get_path()));

    return 0;
}
