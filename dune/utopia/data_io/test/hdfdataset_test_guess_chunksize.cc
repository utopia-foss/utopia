#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <dune/utopia/base.hh>
#include "../hdfchunking.hh"

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

        std::cout << std::endl << "Tests commencing ..." << std::endl;


        // Simple call: typesize, write_extend, max_extend
        auto c1 = guess_chunksize(1, {1, 2, 3}); // 6 Bytes
        // Size is way below CHUNKSIZE_MIN -> single chunk of same size
        assert_equal(c1, {1, 2, 3});

        // Large 1D dataset with typesize 1
        auto c2 = guess_chunksize(1, {1024 * 1024}); // total size: 1M
        // -> single 1M chunk
        assert_equal(c2, {1024 * 1024});

        // Large 1D dataset with larger typesize
        auto c3 = guess_chunksize(8, {1024 * 1024}); // 8M
        // -> eight 1M chunks
        assert_equal(c3, {128 * 1024});

        // Small 1D dataset with large typesize
        auto c4 = guess_chunksize(1024*1024, {4}); // 4M
        // -> four 1M chunks
        assert_equal(c4, {1});
        
        // Small 1D dataset with very large typesize
        auto c5 = guess_chunksize(1024*1024*1024, {4}); // 4G
        // -> four 1G chunks; no other choice
        assert_equal(c5, {1});

        // Small 1D dataset with typesize just above the threshold to allow
        // creation of a size-2 chunk
        auto c6 = guess_chunksize(513*1024, {4}); // slightly above 2M
        // -> four 513k chunks; no other choice
        assert_equal(c6, {1});

        // 2D dataset that has long rows
        auto c7 = guess_chunksize(8, {1, 2048}); // 16k
        // -> no need to chunk
        assert_equal(c7, {1, 2048});


        // End of tests.
        std::cout << "Tests finished." << std::endl << std::endl;
        
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
