/**
 * @brief Tests the constructors of HDFDataset and the associated reference counting.
 *
 * @file hdfdataset_test_lifecycle.cc
 */
#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include "../hdfutilities.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <dune/common/parallel/mpihelper.hh>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace Utopia::DataIO;
using namespace std::literals::chrono_literals;
using hsizevec = std::vector<hsize_t>;

bool operator==(HDFGroup& a, HDFGroup& b)
{
    if (a.get_path() != b.get_path() or a.get_address() != b.get_address())
    {
        return false;
    }
    else
    {
        return true;
    }
}

template <class LHS, class RHS>
void assert_hdfdatasets(LHS& lhs, RHS& rhs)
{
    assert(lhs.get_path() == rhs.get_path());
    assert(lhs.get_id() == rhs.get_id());
    assert(lhs.get_address() == rhs.get_address());
    assert(lhs.get_referencecounter().get() == rhs.get_referencecounter().get());
    assert(&lhs.get_parent() == &rhs.get_parent());
    assert(lhs.get_parent() == rhs.get_parent());
    assert(lhs.get_rank() == rhs.get_rank());
    assert(lhs.get_capacity() == rhs.get_capacity());
    assert(lhs.get_current_extent() == rhs.get_current_extent());
    assert(lhs.get_chunksizes() == rhs.get_chunksizes());
    assert(lhs.get_compresslevel() == rhs.get_compresslevel());
}

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    HDFFile file("dataset_test_lifetime.h5", "w");

    HDFGroup lifecyclegroup(file, "/lifecycletest");
    std::vector<int> data(100, 42);

    HDFDataset first(lifecyclegroup, "first", {100}, {10}, 5);

    HDFDataset first_simple(lifecyclegroup, "first_simple", {}, {10});

    first.write(data.begin(), data.end(), [](int& value) { return value; });
    first_simple.write(data.begin(), data.end(), [](int& value) { return value; });

    assert(H5Iis_valid(first.get_id()));
    assert(H5Iis_valid(first_simple.get_id()));

    assert((*first_simple.get_referencecounter())[first_simple.get_address()] == 1);

    assert((*first.get_referencecounter())[first.get_address()] == 1);

    // copy constructor
    auto copied_first(first);
    assert((*copied_first.get_referencecounter())[copied_first.get_address()] == 2);
    assert_hdfdatasets(first, copied_first);

    // copy assignment
    auto second = first;
    assert((*second.get_referencecounter())[second.get_address()] == 3);
    assert_hdfdatasets(first, second);

    // move assignment
    auto crosscheck(first); // this is needed  for checks
    auto moveassign_from_first = std::move(first);
    assert((*moveassign_from_first.get_referencecounter())[moveassign_from_first.get_address()] == 4);
    assert_hdfdatasets(crosscheck, moveassign_from_first);

    // move constructor
    auto moveconst_second(std::move(second));
    assert((*moveconst_second.get_referencecounter())[moveconst_second.get_address()] == 4);
    assert_hdfdatasets(crosscheck, moveconst_second);

    lifecyclegroup.close();
    file.close();
    file.open("dataset_test_lifetime.h5", "r+");
    lifecyclegroup.open(file, "/lifecycletest");

    // test open method
    HDFDataset<HDFGroup> opened_dataset;
    opened_dataset.open(lifecyclegroup, "first");
    assert(H5Iis_valid(opened_dataset.get_id()) == true);
    assert(opened_dataset.get_current_extent() == hsizevec{100});
    assert(opened_dataset.get_chunksizes() == hsizevec{10});
    assert(opened_dataset.get_capacity() == hsizevec{100});

    // test simple open method
    HDFDataset<HDFGroup> opened_dataset_simple;
    opened_dataset_simple.open(lifecyclegroup, "first_simple");
    assert(H5Iis_valid(opened_dataset_simple.get_id()) == true);
    assert(opened_dataset_simple.get_current_extent() == hsizevec{100});
    assert(opened_dataset_simple.get_chunksizes() == hsizevec{10});
    assert(opened_dataset_simple.get_capacity() == hsizevec{H5S_UNLIMITED});

    return 0;
}
