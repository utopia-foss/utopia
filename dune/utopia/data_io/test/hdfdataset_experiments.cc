#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <iostream>

using namespace Utopia::DataIO;

int main()
{
    HDFFile file("testfile.h5", "w");
    auto contset = file.open_dataset("/containerdataset", {100}, {5});
    auto stringset = file.open_dataset("/stringdataset", {100}, {5});
    auto ptrset = file.open_dataset("/pointerdataset", {100}, {5});
    auto scalarset = file.open_dataset("/scalardataset", {100}, {5});

    contset->write(std::vector<double>(10, 3.14));

    stringset->write(std::string("niggastring"));

    std::shared_ptr<double> ptr(new double[30]);
    for (std::size_t i = 0; i < 30; ++i)
    {
        ptr.get()[i] = 3.14;
    }

    ptrset->write(ptr.get(), {30});

    scalarset->write(5);

    return 0;
}