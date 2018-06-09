#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <dune/utopia/base.hh>
#include "../hdffile.hh"
#include "../hdfdataset.hh"

using namespace Utopia::DataIO;

template<typename T>
void assert_equal(const std::vector<T> a, const std::vector<T> b) {
    // Evaluate whether equal
    const bool equal = std::equal(a.begin(), a.end(), b.begin(), b.end());

    // Create some output
    std::cout << "Vectors" << std::endl << "   ";
    for (auto element: a)
        std::cout << element << ",  ";
    std::cout << std::endl << "   ";
    for (auto element: b)
        std::cout << element << ",  ";
    std::cout << std::endl;

    if (equal == true)
        std::cout << "are equal." << std::endl << std::endl;
    else
        std::cout << "are NOT equal." << std::endl << std::endl;

    assert(equal);
}

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // -- Setup -- //
        // Initialize a temporary file the datasets are written to
        std::string path = "dataset_chunksize_test.h5";
        std::cout << "Creating temporary file at " << path << std::endl;
        HDFFile file(path, "w");


        // -- Tests -- //
        std::cout << std::endl << "Tests commencing ..." << std::endl;

        // Simple call
        auto chunks = guess_chunksize({1, 2, 3},  1); // extend and typesize
        assert_equal(chunks, {1, 2, 3});

        std::cout << "Tests finished." << std::endl << std::endl;


        // -- Cleanup -- //
        std::cout << "Closing temporary file ..." << std::endl;
        file.close();
        std::cout << "  Done." << std::endl;

        std::cout << "Removing temporary file ..." << std::endl;
        std::remove(path.c_str());
        std::cout << "  Done." << std::endl;

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch(...) {
        std::cerr << "Exception thrown!" << std::endl;
        return 1;
    }
}
