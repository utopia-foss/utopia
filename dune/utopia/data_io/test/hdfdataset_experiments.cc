#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <iostream>

using namespace Utopia::DataIO;
// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

void write()
{
    // make data used later

    std::array<int, 4> arr{{0, 1, 2, 3}};
    std::array<int, 4> arr2{{4, 5, 6, 7}};

    std::shared_ptr<double> ptr(new double[5]);
    for (std::size_t i = 0; i < 5; ++i)
    {
        ptr.get()[i] = 3.14;
    }

    std::vector<Point> points(100);
    for (int i = 0; i < 100; ++i)
    {
        points[i].x = 3.14;
        points[i].y = 3.14 + 1;
        points[i].z = 3.14 + 2;
    }

    // make hdfobjects needed
    HDFFile file("testfile.h5", "w");
    auto contset = file.open_dataset("/containerdataset", {100}, {5});
    auto contcontset = file.open_dataset("/containercontainerdataset", {100}, {5});
    auto stringset = file.open_dataset("/stringdataset", {100}, {5});
    auto ptrset = file.open_dataset("/pointerdataset", {100}, {5});
    auto scalarset = file.open_dataset("/scalardataset", {100}, {5});
    auto twoDdataset = file.open_dataset("/2ddataset", {10, 100}, {1, 5});
    auto adapteddataset = file.open_dataset("/adapteddataset", {500}, {50});

    contset->write(std::vector<double>(10, 3.14));
    contset->write(std::vector<double>(10, 6.28));
    contset->write(std::vector<double>(10, 9.42));

    contcontset->write(std::vector<std::array<int, 4>>(20, arr));
    contcontset->write(std::vector<std::array<int, 4>>(20, arr2));

    stringset->write(std::string("niggastring"));

    for (std::size_t i = 0; i < 25; ++i)
    {
        stringset->write(std::to_string(i));
    }

    ptrset->write(ptr.get(), {5});

    for (std::size_t j = 2; j < 4; ++j)
    {
        for (std::size_t i = 0; i < 5; ++i)
        {
            ptr.get()[i] = j * 3.14;
        }
        ptrset->write(ptr.get(), {5});
    }

    for (int i = 0; i < 5; ++i)
    {
        scalarset->write(i);
    }

    for (std::size_t i = 0; i < 6; ++i)
    {
        twoDdataset->write(std::vector<std::size_t>(100, i));
    }

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.x; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.y; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.z; });
}

void read()
{
    HDFFile file("testfile.h5", "r");
    auto contset = file.open_dataset("containerdataset");
    auto contcontset = file.open_dataset("containercontainerdataset");
    auto stringset = file.open_dataset("stringdataset");
    auto ptrset = file.open_dataset("pointerdataset");
    auto scalarset = file.open_dataset("scalardataset");
    auto twoDdataset = file.open_dataset("2ddataset");
    auto adapteddataset = file.open_dataset("adapteddataset");

    auto [shape, data] = contset->read<std::vector<double>>();
    std::cout << shape << std::endl;
    std::cout << data << std::endl;

    auto [shape2, data2] = contcontset->read<std::vector<std::array<int, 4>>>();

    std::cout << shape2 << std::endl;
    std::cout << data2 << std::endl;

    auto [shape3, data3] = stringset->read<std::vector<std::string>>();
    for (auto& str : data3)
    {
        if (str != "")
        {
            std::cout << str << ",";
        }
    }
    std::cout << std::endl;

    auto [shape4, data4] = stringset->read<std::string>();
    std::cout << data4 << std::endl;
}
int main()
{
    write();
    read();
    return 0;
}