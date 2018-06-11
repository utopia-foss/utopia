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

        // -- Without max_extend -- //

        // Simple call: typesize, write_extend, [max_extend]
        // Size is way below CHUNKSIZE_MIN -> single chunk of same size
        assert_equal(guess_chunksize(1, {1, 2, 3}), // 6 Bytes total size
                     {1, 2, 3});

        // Large 1D dataset with typesize 1, no max_extend
        // -> naive optimization
        assert_equal(guess_chunksize(1, {1024 * 1024}), // 1M
                     {128 * 1024});

        // Large 1D dataset with larger typesize, still no max_extend
        // -> naive optimization, smaller chunks due to higher typesize
        assert_equal(guess_chunksize(8, {1024 * 1024}), // 8M
                     {32 * 1024});

        // Small 1D dataset with large typesize, no_
        // -> four 1M chunks; no other choice
        assert_equal(guess_chunksize(1024*1024, {4}), // 1M
                     {1});
        
        // Small 1D dataset with very large typesize
        // -> four 1G chunks; no other choice
        assert_equal(guess_chunksize(1024*1024*1024, {4}), // 4G
                     {1});

        // Small 1D dataset with typesize just above the threshold to allow
        // creation of a size-2 chunk
        // -> four 513k chunks; no other choice
        assert_equal(guess_chunksize(513*1024, {4}), // slightly above 2M
                     {1});

        // 2D dataset that has long rows
        // -> naive optimization
        assert_equal(guess_chunksize(8, {1, 2048}), // 16k
                     {1, 1024});


        // -- With max_extend -- //

        // Fits CHUNKSIZE_MAX, already reached max_extend
        // -> skip naive optimization, fits I/O operation into chunk, done
        assert_equal(guess_chunksize(1, {1024 * 1024},
                                     {1024 * 1024}),
                     {1024 * 1024});

        // Fits CHUNKSIZE_MAX, not yet reached max_extend
        // -> skip naive optimization, fits I/O operation into chunk, but the
        //    chunk cannot be further enlarged
        assert_equal(guess_chunksize(1, {1024 * 1024},
                                     {16 * 1024 * 1024}),
                     {1024 * 1024});

        // Smaller, still fits CHUNKSIZE_MAX, not yet reached max_extend
        // -> skip naive optimization, fits I/O operation into chunk, but there
        //    is room for further optimization
        assert_equal(guess_chunksize(1, {128 * 1024},
                                     {16 * 1024 * 1024}),
                     {1024 * 1024});


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
