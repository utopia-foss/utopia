#ifndef HDFCHUNKING_HH
#define HDFCHUNKING_HH

#include <hdf5.h>
#include <hdf5_hl.h>
#include <numeric>
#include <cmath>

// TODO which of these includes are needed?

namespace Utopia {
namespace DataIO {

/**
 * @brief   Try to guess a good chunksize for a dataset
 * @detail  The premise is that a single write operation should be as fast
 *          as possible, i.e. that it occurs within one chunk. Also, if a
 *          maximum dataset size is known, it is taken
 *          into account to determine
 *
 * @param   typesize        The size of each element in bytes
 * @param   write_extend    The extend of one write operation. The rank of the
 *                          dataset is extracted from this argument.
 * @param   max_extend      The maximum extend the dataset can have. If given,
 *                          first the write_extend is used to determine the
 *                          chunks along the major axis; then, max_extend is
 *                          used to increase chunk sizes along the minor axes.
 * @param   CHUNKSIZE_MAX   largest chunksize; should not exceed 1MiB too much,
 *                          or, more precisely: should fit into the chunk cache
 *                          which (by default) is 1MiB large 
 * @param   CHUNKSIZE_MIN   smallest chunksize; should be above a few KiB
 * @param   CHUNKSIZE_BASE  base factor for creating target chunksize
 */
// TODO use log messages instead of std::cout
// TODO is it reasonable to use const here?
const std::vector<hsize_t>
    guess_chunksize(const hsize_t typesize,
                    const std::vector<hsize_t> write_extend,
                    const std::vector<hsize_t> max_extend = {},
                    const unsigned int CHUNKSIZE_MAX = 1048576,  // 1M
                    const unsigned int CHUNKSIZE_MIN = 8192,     // 8k
                    const unsigned int CHUNKSIZE_BASE = 16384    // 16k
                    )
{
    // -- Initialisations -- //    
    // Helper lambda for calculating the product of vector entries
    auto product = [](const std::vector<hsize_t> vec) {
        return std::accumulate(vec.begin(), vec.end(), 1, std::multiplies<>());
    };

    // Helper lambda for string representation of vectors
    auto vec2str = [](const std::vector<hsize_t> vec) {
        std::stringstream s;
        s << "( ";
        for (auto element: vec)
            s << element << " ";
        s << ")";
        return s.str();
    };

    // Selected variable initialisations
    bool dset_finite;


    // -- Check correctness of arguments and extract some info -- //
    // Get the rank
    auto rank = write_extend.size();

    // For scalar datasets, chunking is not available
    if (rank == 0) {
        throw std::invalid_argument("Cannot guess chunksize for scalar "
                                    "dataset!");
    }
    

    // Find out if the max_extend is given
    if (max_extend.size()) {
        // Is given, yes

        // Check that it matches the rank
        if (max_extend.size() != rank) {
            throw std::invalid_argument("Argument 'max_extend' does not have "
                                        "the same dimensionality as the rank "
                                        "of this dataset, as extracted from "
                                        "the write_extend argument.");
            // TODO add information on rank
        }

        // Need to check whether any dataset dimension can be infinitely long
        dset_finite = (std::find(max_extend.begin(), max_extend.end(), 0)
                       == max_extend.end());
        // TODO Check if H5S_UNLIMITED is a valid value in max_extend?
    }


    std::cout << "guessing chunksize for:" << std::endl;
    std::cout << "  typesize:     " << typesize << std::endl;
    std::cout << "  write_extend: " << vec2str(write_extend) << std::endl;
    std::cout << "  max_extend:   " << vec2str(max_extend) << std::endl;
    std::cout << "  rank:         " << rank << std::endl;
    std::cout << "  finite dset?  " << dset_finite << std::endl;


    // -- For the simple cases, evaluate the chunksize directly -- //
    // For large typesizes, each chunk needs to contain only a single element.
    // Chunks that extend to more than one element require a typesize smaller
    // than half the maximum chunksize.
    if (typesize > CHUNKSIZE_MAX / 2) {
        std::cout << "  -> type size >= 1/2 max. chunksize" << std::endl;
        return std::vector<hsize_t>(rank, 1);
    }



    // For a (maximally extended) dataset that is smaller than CHUNKSIZE_MAX,
    // only need a single chunk
    if (max_extend.size()
        && (typesize * product(max_extend) < CHUNKSIZE_MAX))
    {
        std::cout << "  -> maximally extended dataset fits into one chunk"
                  << std::endl;
        return std::vector<hsize_t>(max_extend);
    }



    // -- Optimize for one I/O operation fitting into chunk -- //
    // 


    // Create non-const copies of the given arguments
    std::vector<hsize_t> extd(write_extend);
    std::vector<hsize_t> max_extd(max_extend);

    // Extend values can also be 0, indicating unlimited extend of that
    // dimension. To not run into further problems, guess a value for those.
    // h5py uses 1024 here, let's do the same
    std::replace(extd.begin(), extd.end(), 0, 1024);
    // TODO figure out why this specific value is used

    // Determine the size (in bytes) of a write operation with this extend
    auto bytes_extd = typesize * product(extd);
    // NOTE The std::accumulate is used for calculating the product of entries

    // Calculate the target chunksize: base * 2^log10(size/max_size)
    double bytes_target = (CHUNKSIZE_BASE
                           * std::pow(2,
                                      std::log10(bytes_extd/(1024.*1024))));
    // NOTE this is a double as it is more convenient to calculate with ...

    // Ensure the target chunk size is between CHUNKSIZE_MIN and CHUNKSIZE_MAX
    // in order to not choose too large or too small chunks
    if (bytes_target > CHUNKSIZE_MAX) {
        bytes_target = CHUNKSIZE_MAX;
    }
    else if (bytes_target < CHUNKSIZE_MIN) {
        bytes_target = CHUNKSIZE_MIN;
    }

    std::cout << "  bytes_extd:   " << bytes_extd << std::endl;
    std::cout << "  bytes_target: " << bytes_target << std::endl;

    // Create the temporary target vector that will store the chunksize values.
    // It starts with a copy of the extend values. After optimization, a const
    // version of this is created and returned.
    std::vector<hsize_t> _chunks(extd);

    // ... and a variable that will store the size (in bytes) of this specific
    // chunk configuration
    unsigned long int bytes_chunks;

    /* Now optimize the chunks for each dimension by repeatedly looping over
     * the vector and dividing the values by two (rounding up).
     *
     * The loop is left when the following condition is fulfilled:
     *   smaller than target chunk size OR within 50% of target chunk size
     *   AND
     *   smaller than maximum chunk size
     * Also, if the typesize is larger 
     * 
     * NOTE:
     * Limit the optimization to 23 iterations per dimension; usually, we will
     * leave the loop much earlier; the _mean_ extend of the dataset would have
     * to be ~8M entries _per dimension_ to exhaust this optimization loop.
     */
    std::cout << "optimization:" << std::endl;
    for (unsigned int i=0; i < 23 * rank; i++)
    {
        // With the current values of the chunks, calculate the chunk size
        bytes_chunks = typesize * product(_chunks);
        std::cout << "  bytes_chunks: " << bytes_chunks;

        // If close enough to target size, optimization is finished
        if ((   bytes_chunks <= bytes_target
             || (std::abs(bytes_chunks - bytes_target) / bytes_target < 0.5))
            && bytes_chunks <= CHUNKSIZE_MAX)
        {
            std::cout << "  -> close enough to target size now" << std::endl;
            break;
        }

        // Divide the chunk size of the current axis by two, rounding upwards
        std::cout << "  -> reducing size of dim " << i%rank << std::endl;
        _chunks[i % rank] = 1 + ((_chunks[i % rank] - 1) / 2);
        // NOTE integer division fun; can do this because all are unsigned
        // and the chunks entry is always nonzero
    }

    // If the 

    // Create a const version of the temporary chunks vector
    const std::vector<hsize_t> chunks(_chunks);
    // TODO is this the best way to do this?

    return chunks;
}


} // namespace DataIO
} // namespace Utopia
#endif // HDFCHUNKING_HH
