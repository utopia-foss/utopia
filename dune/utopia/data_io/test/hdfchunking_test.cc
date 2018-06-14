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

        // Very small 3D dataset without max_extend
        // -> I/O fits into chunk, but dataset is infinite and below base
        //    chunksize -> will optimize towards base chunksize
        assert_equal(guess_chunksize(1, {1, 2, 3}), // 6 Bytes total size
                     {32, 64, 96}); // 192k, close enough to base chunksize

        // Again, without that optimization
        assert_equal(guess_chunksize(1, {1, 2, 3}, {}, false),
                     {1, 2, 3}); // stays the same

        // Large 1D dataset with typesize 1, no max_extend
        // -> fit I/O operation into maximum chunksize
        assert_equal(guess_chunksize(1, {1024 * 1024}), // 1M
                     {1024 * 1024});

        // Large 1D dataset with larger typesize, still no max_extend
        // -> I/O will not fit into single chunk -> use optimization
        assert_equal(guess_chunksize(8, {1024 * 1024}), // 8M
                     {128 * 1024}); // 8 such chunks for a single I/O

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
        // -> fits into chunk, but is infinite -> further optimization
        assert_equal(guess_chunksize(8, {1, 2048}), // 16k
                     {4, 8192}); // 256k == base chunksize

        // Again, without optimization
        assert_equal(guess_chunksize(8, {1, 2048}, {}, false), // 16k
                     {1, 2048}); // stays the same

        // Tests whether dimensions get optimized in the right order
        assert_equal(guess_chunksize(1, {8, 8, 8, 8, 8}), // 32k
                     {8, 8, 16, 16, 16}); // 256k, last axes first

        // Tests whether dimensions get optimized in the right order
        assert_equal(guess_chunksize(1, {50, 50, 50}), // 125k
                     {50, 50, 100}); // 250k, last axes first

        // ... unless the chunksize is already >= base chunksize
        assert_equal(guess_chunksize(1, {64, 64, 64}), // 256k == base
                     {64, 64, 64}); // stays the same


        // -- With max_extend -- //

        // 1D, io_extend fits, already reaching max_extend
        // -> no optimization needed
        assert_equal(guess_chunksize(1, {1024 * 1024},    // 1M
                                     {1024 * 1024}),      // 1M
                     {1024 * 1024});                      // 1M

        // 1D, io_extend fits
        // -> cannot further enlarge chunk, although max_extend not reached
        assert_equal(guess_chunksize(1, {1024 * 1024},    // 1M
                                     {16 * 1024 * 1024}), // 1M
                     {1024 * 1024});                      // 1M

        // 1D, io_extend smaller, max_extend == max. chunksize
        // -> there is room for optimization: can extend chunk to 1M to cover
        //    the whole dataset.
        assert_equal(guess_chunksize(1, {128 * 1024},     // 128k
                                     {1024 * 1024}),      // 1M
                     {1024 * 1024});                      // 1M

        // 1D, io_extend smaller, max_extend > max. chunksize
        // -> can extend to max. chunksize, fitting exactly 16 chunks in.
        assert_equal(guess_chunksize(1, {128 * 1024},     // 128k
                                     {16 * 1024 * 1024}), // 16M
                     {1024 * 1024});                      // 1M

        // 1D, io_extend larger, max_extend > max. chunksize
        // -> can extend to max. chunksize, fitting exactly 16 chunks in.
        assert_equal(guess_chunksize(1, {2048 * 1024},    // 2M
                                     {16 * 1024 * 1024}), // 16M
                     {1024 * 1024});                      // 1M

        // 1D, io_extend fits, max_extend infinite
        // -> not below base chunksize; nothing to do
        assert_equal(guess_chunksize(1, {1024 * 1024},    // 1M
                                     {0}),                // inf
                     {1024 * 1024});                      // 1M
        
        // 1D, io_extend smaller, max_extend inf
        // -> below max. chunksize -> optimize towards that
        assert_equal(guess_chunksize(1, {128 * 1024},     // 128k
                                     {0}),                // inf
                     {1024 * 1024});                      // 1M
        
        // 1D, io_extend smaller, max_extend inf, opt_inf_dims disabled
        // -> below base. chunksize -> optimize towards base
        assert_equal(guess_chunksize(1, {128 * 1024},     // 128k
                                     {0},                 // inf
                                     true, false), // avoid_low, opt_inf
                     {256 * 1024});                       // 256k
        
        // 1D, io_extend smaller, max_extend inf, opt_inf_dims disabled
        // -> above base. chunksize -> do nothing
        assert_equal(guess_chunksize(1, {345 * 1024},     // 345k
                                     {0},                 // inf
                                     true, false), // avoid_low, opt_inf
                     {345 * 1024});                       // 345k

        // 2D dataset, io_extend smaller, max_extend > max chunksize
        // -> extend first axis as far as possible
        assert_equal(guess_chunksize(1, {1, 512, 512},    // 256k
                                     {512, 512, 512}),    // 128M
                     {4, 512, 512});                      // 1M

        // ... with other (rather unfortunate) values
        // -> optimize for io_extend to fit in
        assert_equal(guess_chunksize(1, {1, 123, 456},    // ~54k
                                     {512, 512, 512}),    // max_extend
                     {16, 123, 456});                     // ~880k
        // TODO there is room for improvement here! better: {4, 512, 512}

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
