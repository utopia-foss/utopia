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
        auto c1 = guess_chunksize({1, 2, 3},  1); // extend and typesize
        // Size is way below CHUNKSIZE_MIN -> single chunk of same size
        assert_equal(c1, {1, 2, 3});

        // Large 1D dataset with typesize 1
        auto c2 = guess_chunksize({1024 * 1024}, 1); // 1M
        // -> single 1M chunk
        assert_equal(c2, {1024 * 1024});

        // Large 1D dataset with larger typesize
        auto c3 = guess_chunksize({1024 * 1024}, 8); // 8M
        // -> eight 1M chunks
        assert_equal(c3, {128 * 1024});

        // Small 1D dataset with large typesize
        auto c4 = guess_chunksize({4}, 1024*1024); // 4M
        // -> four 1M chunks
        assert_equal(c4, {1});
        
        // Small 1D dataset with very large typesize
        auto c5 = guess_chunksize({4}, 1024*1024*1024); // 4G
        // -> four 1G chunks; no other choice
        assert_equal(c5, {1});

        // Small 1D dataset with typesize just above the threshold to allow
        // creation of a size-2 chunk
        auto c6 = guess_chunksize({4}, 513*1024); // slightly above 2M
        // -> four 513k chunks; no other choice
        assert_equal(c6, {1});

        // 2D, ...

        // End of tests.
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
