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

/// Assert that two Dataset instances have the same public members
/** \tparam LHS Left hand side dataset type
 *  \tparam RHS Right hand side dataset type
 *  Template parameters are necessary because HDFDataset is a template
 */
template <typename T>
bool operator==(std::vector<T>& a, std::vector<T>& b)
{
    if (a.size() == b.size())
    {
        return false;
    }
    else
    {
        if (std::is_floating_point<T>::value)
        {
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (std::abs((a[i] - b[i]) / max(a[i], b[i])) < 1e-16)
                {
                    return false;
                }
            }
        }
        else
        {
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (a[i] != b[i])
                {
                    return false;
                }
            }
        }
        return true;
    }
}

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
    assert(lhs.get_referencecounter() == rhs.get_referencecounter());
    assert(&lhs.get_parent() == &rhs.get_parent());
    assert(lhs.get_parent() == rhs.get_parent());
    assert(lhs.get_rank() == rhs.get_rank());
    assert(lhs.get_capacity() == rhs.get_capacity());
    assert(lhs.get_current_extend() == rhs.get_current_extend());
    assert(lhs.get_chunksizes() == rhs.get_chunksizes());
    assert(lhs.get_compresslevel() == rhs.get_compresslevel());
}

int main()
{
    HDFFile file("dataset_test_lifetime.h5", "w");

    HDFGroup lifecyclegroup(*file.get_basegroup(), "lifecycletest");
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

    // lifecyclegroup.close();
    // file.close();
    // file.open("dataset_test_lifetime.h5", "r+");
    // lifecyclegroup.open(*file.get_basegroup(), "lifecycletest");
    // test open method
    HDFDataset<HDFGroup> opened_dataset;
    opened_dataset.open(lifecyclegroup, "first", {100}, {10}, 5);
    assert(H5Iis_valid(opened_dataset.get_id()) == true);

    // test simple open method
    HDFDataset<HDFGroup> opened_dataset_simple;
    opened_dataset_simple.open(lifecyclegroup, "first_simple");
    assert(H5Iis_valid(opened_dataset_simple.get_id()) == true);
    return 0;
}
