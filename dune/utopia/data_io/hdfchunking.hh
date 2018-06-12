#ifndef HDFCHUNKING_HH
#define HDFCHUNKING_HH

#include <hdf5.h>
#include <numeric>
#include <cmath>
// TODO which of these includes are needed?

/// Container that holds indices
using IdxCont = std::vector<unsigned short>;

namespace Utopia {
namespace DataIO {

// -- Helper functions -- //

// TODO is there a better place to put this?
/// Finds all indices of elements in a vector that matches the given predicate
template<typename Cont, typename Predicate>
IdxCont __find_all_idcs (Cont &vec, Predicate pred) {
    // Create the return container
    IdxCont idcs;

    // Repeatedly start iterating over the vector until reached the end
    auto iter = vec.begin();
    while ((iter = std::find_if(iter, vec.end(), pred)) != vec.end())
    {
        // Add the value of the iterator to the indices vector
        idcs.push_back(std::distance(vec.begin(), iter));

        // Increment iterator to continue with next element
        iter++;
    }

    return idcs;
};


// -- Optimization algorithms -- //

/// Optimizes the chunks with a naive algorithm 
template<typename Cont>
void __opt_chunks_naive(Cont &chunks,
                        hsize_t bytes_io,
                        const hsize_t typesize,
                        const unsigned int CHUNKSIZE_MAX,
                        const unsigned int CHUNKSIZE_MIN,
                        const unsigned int CHUNKSIZE_BASE)
{
    // Helper lambda for calculating the product of vector entries
    auto product = [](const std::vector<hsize_t> vec) {
        return std::accumulate(vec.begin(), vec.end(), 1, std::multiplies<>());
    };

    // Calculate the rank
    auto rank = chunks.size();

    // Calculate the target chunksize. Formula:  base * 2^log10(size/max_size)
    double bytes_target = (CHUNKSIZE_BASE
                           * std::pow(2,
                                      std::log10(bytes_io/CHUNKSIZE_MAX)));
    // NOTE this is a double as it is more convenient to calculate with ...

    // Ensure the target chunk size is between CHUNKSIZE_MIN and CHUNKSIZE_MAX
    // in order to not choose too large or too small chunks
    if (bytes_target > CHUNKSIZE_MAX) {
        bytes_target = CHUNKSIZE_MAX;
    }
    else if (bytes_target < CHUNKSIZE_MIN) {
        bytes_target = CHUNKSIZE_MIN;
    }

    std::cout << "  target chunk size: " << bytes_target
              << "  (" << bytes_target/1024 << " kiB) " << std::endl;


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
     * 
     * NOTE:
     * Limit the optimization to 23 iterations per dimension; usually, we will
     * leave the loop much earlier; the _mean_ extend of the dataset would have
     * to be ~8M entries _per dimension_ to exhaust this optimization loop.
     */
    std::cout << "  => starting naive optimization ..." << std::endl;
    for (unsigned int i=0; i < 23 * rank; i++)
    {
        // With the current values of the chunks, calculate the chunk size
        bytes_chunks = typesize * product(chunks);
        std::cout << "    chunk size:      " << bytes_chunks
                  << " B  (" << bytes_chunks/1024 << " kiB) ";

        // If close enough to target size, optimization is finished
        if ((   bytes_chunks <= bytes_target
             || (std::abs(bytes_chunks - bytes_target) / bytes_target < 0.5))
            && bytes_chunks <= CHUNKSIZE_MAX)
        {
            std::cout << "  -> close enough to target size" << std::endl;
            break;
        }

        // Divide the chunk size of the current axis by two, rounding upwards
        std::cout << "  -> reducing size of dim " << i%rank << std::endl;
        chunks[i % rank] = 1 + ((chunks[i % rank] - 1) / 2);
        // NOTE integer division fun; can do this because all are unsigned
        // and the chunks entry is always nonzero
    }

    return;
}


/// Optimizes the chunks using the max_extend information
template<typename Cont, typename IdxCont=std::vector<unsigned short>>
void __opt_chunks_with_max_extend(Cont &chunks,
                                  const Cont &max_extend,
                                  [[maybe_unused]] const hsize_t typesize,
                                  [[maybe_unused]] const unsigned int CHUNKSIZE_MAX)
{
    // Helper lambda for calculating the product of vector entries
    [[maybe_unused]] auto product = [](const std::vector<hsize_t> vec) {
        return std::accumulate(vec.begin(), vec.end(), 1, std::multiplies<>());
    };

    // Helper lambda to calculate diff between two vectors, a-b
    [[maybe_unused]] auto diff = [](Cont &a, Cont &b) {
        // Create the return vector
        Cont _diff(a);

        for (unsigned int i=0; i < _diff.size(); i++) {
            _diff[i] -= b[i];
        }
        
        return _diff;
    };

    // -- Parse axes -- //

    // Available axes
    IdxCont axes(chunks.size());
    std::iota(axes.begin(), axes.end(), 0);

    // Determine the infinite axes
    auto axes_inf = __find_all_idcs(max_extend,
                                    [](auto extd){return extd == 0;});
    // The chunk size along these axes should be as small as possible

    // Determine the finite axes
    auto axes_fin = __find_all_idcs(max_extend,
                                    [](auto extd){return extd != 0;});

    // For finite axes, determine the diff between chunks and max_extend
    Cont remainder(chunks.size(), -1);
    for (auto idx: axes_fin) {
        remainder[idx] = max_extend[idx] - chunks[idx];
    }

    // Among the finite axes, determine the axes that can still be filled,
    // i.e. those where the chunk size does not reach the max_extend
    [[maybe_unused]] IdxCont axes_fillable;
    for (auto idx: axes_fin) {
        if (remainder[idx]) {
            // TODO Continue here            
        }
    }

    // -- Done -- //
    return;
}


/**
 * @brief   Try to guess a good chunksize for a dataset
 * @detail  The premise is that a single write operation should be as fast
 *          as possible, i.e. that it occurs within one chunk. Also, if a
 *          maximum dataset extend is known, it is taken into account to
 *          determine more favourable chunk sizes.
 *
 * @param   typesize        The size of each element in bytes
 * @param   io_extend       The extend of one I/O operation. The rank of the
 *                          dataset is extracted from this argument. The
 *                          algorithm is written to make an I/O operation of
 *                          this extend use as few chunks as possible.
 * @param   max_extend      The maximum extend the dataset can have. If given,
 *                          the chunk size is increased along the open axes to
 *                          spread evenly and fill the max_extend as best as
 *                          as possible.
 * @param   CHUNKSIZE_MAX   largest chunksize; should not exceed 1MiB too much,
 *                          or, more precisely: should fit into the chunk cache
 *                          which (by default) is 1MiB large 
 * @param   CHUNKSIZE_MIN   smallest chunksize; should be above a few KiB
 * @param   CHUNKSIZE_BASE  base factor for creating target chunksize if the
 *                          dataset has no maximum extend given
 */
// TODO use log messages instead of std::cout
// TODO is it reasonable to use const here?
const std::vector<hsize_t>
    guess_chunksize(const hsize_t typesize,
                    const std::vector<hsize_t> io_extend,
                    const std::vector<hsize_t> max_extend = {},
                    const unsigned int CHUNKSIZE_MAX = 1048576,  // 1M
                    const unsigned int CHUNKSIZE_MIN = 8192,     // 8k
                    const unsigned int CHUNKSIZE_BASE = 131072)  // 128k
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
    auto rank = io_extend.size();

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
                                        "the io_extend argument.");
            // TODO add information on rank
        }

        // Need to check whether any dataset dimension can be infinitely long
        dset_finite = (std::find(max_extend.begin(), max_extend.end(), 0)
                       == max_extend.end());
        // TODO Check if H5S_UNLIMITED is a valid value in max_extend?
    }


    std::cout << "Guessing appropriate chunk size using:" << std::endl;
    std::cout << "  io_extend:         " << vec2str(io_extend) << std::endl;
    std::cout << "  max_extend:        " << vec2str(max_extend) << std::endl;
    std::cout << "  rank:              " << rank << std::endl;
    std::cout << "  finite dset?       " << dset_finite << std::endl;
    std::cout << "  typesize:          " << typesize << " B" << std::endl;
    std::cout << "  max. chunksize:    "
              << CHUNKSIZE_MAX/1024 << " kiB" << std::endl;
    std::cout << "  min. chunksize:    "
              << CHUNKSIZE_MIN/1024 << " kiB" << std::endl;
    std::cout << "  base chunksize:    "
              << CHUNKSIZE_BASE/1024 << " kiB" << std::endl;


    // -- For the simple cases, evaluate the chunksize directly -- //

    // For large typesizes, each chunk can at most contain a single element.
    // Chunks that extend to more than one element require a typesize smaller
    // than half the maximum chunksize.
    if (typesize > CHUNKSIZE_MAX / 2) {
        std::cout << "  -> type size >= 1/2 max. chunksize" << std::endl;
        return std::vector<hsize_t>(rank, 1);
    }


    // For a (maximally extended) dataset that is smaller than CHUNKSIZE_MAX,
    // can only have (and only need) a single chunk
    if (max_extend.size()
        && (typesize * product(max_extend) < CHUNKSIZE_MAX))
    {
        std::cout << "  -> maximally extended dataset will fit into one chunk"
                  << std::endl;
        return std::vector<hsize_t>(max_extend);
    }


    // -- Optimize for one I/O operation fitting into chunk -- //

    // Create the temporary target vector that will store the chunksize values.
    // It starts with a copy of the extend values for I/O operations.
    std::vector<hsize_t> _chunks(io_extend);

    // Determine the size (in bytes) of a write operation with this extend
    auto bytes_io = typesize * product(io_extend);
    std::cout << "  I/O op. size:      " << bytes_io
              << " B  (" << bytes_io/1024 << " kiB) " << std::endl;

    // Determine if an I/O operation fits into a single chunk
    bool fits_into_chunk = (bytes_io <= CHUNKSIZE_MAX);
    std::cout << "  fits into chunk?   " << fits_into_chunk << std::endl;

    // If it does not fit into the chunk or if the maximum extend is not known,
    // or if it is infinite, need to start optimize here already
    if (!fits_into_chunk || !max_extend.size() || !dset_finite) {
        __opt_chunks_naive(_chunks, bytes_io, typesize,
                           CHUNKSIZE_MAX, CHUNKSIZE_MIN, CHUNKSIZE_BASE);
    }
    // else: no need to optimize for a single write operation


    // -- If given, take the max_extend into account -- //
    // This is only possible if the current chunk size is not already close to
    // the upper limit, CHUNKSIZE_MAX
    if (max_extend.size()
        && (typesize * product(_chunks) <= CHUNKSIZE_MAX/2))
    {
        std::cout << "  can further optimize using max_extend info ..."
                  << std::endl;

        __opt_chunks_with_max_extend(_chunks, max_extend,
                                     typesize, CHUNKSIZE_MAX);
    }
    // else: no further optimization possible



    // -- Done. -- //

    // Create a const version of the temporary chunks vector
    const std::vector<hsize_t> chunks(_chunks);
    // TODO is this the best way to do this?

    return chunks;
}


} // namespace DataIO
} // namespace Utopia
#endif // HDFCHUNKING_HH
