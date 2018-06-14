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

/// Optimizes the chunks along all axes to find a good default
// TODO doxygen
template<typename Cont>
void __opt_chunks_target(Cont &chunks,
                         double bytes_target,
                         const hsize_t typesize,
                         const unsigned int CHUNKSIZE_MAX,
                         const unsigned int CHUNKSIZE_MIN,
                         const bool larger_high_dims)
{
    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
    };


    std::cout << "  => starting optimization towards target size: "
              << bytes_target << " B  (" << bytes_target/1024 << " kiB) ..."
              << std::endl;

    // Ensure the target chunk size is between CHUNKSIZE_MIN and CHUNKSIZE_MAX
    // in order to not choose too large or too small chunks
    if (bytes_target > CHUNKSIZE_MAX) {
        bytes_target = CHUNKSIZE_MAX;

        std::cout << "    target size too large. New target size: "
                  << bytes_target << " B  (" << bytes_target/1024 << " kiB) "
                  << std::endl;
    }
    else if (bytes_target < CHUNKSIZE_MIN) {
        bytes_target = CHUNKSIZE_MIN;

        std::cout << "    target size too small. New target size: "
                  << bytes_target << " B (" << bytes_target/1024 << " kiB) "
                  << std::endl;
    }

    // ... and a variable that will store the size (in bytes) of this specific
    // chunk configuration
    size_t bytes_chunks;

    // Calculate the rank (need it to know iteration -> dim mapping)
    auto rank = chunks.size();

    /* Now optimize the chunks for each dimension by repeatedly looping over
     * the vector and dividing the values by two (rounding up).
     *
     * The loop is left when the following condition is fulfilled:
     *   within 50% of target chunk size
     *   AND
     *   within bounds of minimum and maximum chunk size
     * 
     * NOTE:
     * Limit the optimization to 23 iterations per dimension; usually, we will
     * leave the loop much earlier; the _mean_ extend of the dataset would have
     * to be ~8M entries _per dimension_ to exhaust this optimization loop.
     */
    for (unsigned short i=0; i < 23 * rank; i++)
    {
        // With the current values of the chunks, calculate the chunk size
        bytes_chunks = bytes(chunks);
        std::cout << "    chunk size:      " << bytes_chunks
                  << " B  (" << bytes_chunks/1024 << " kiB) ";

        // If close enough to target size, optimization is finished
        if (   (std::abs(bytes_chunks - bytes_target) / bytes_target < 0.5)
            && bytes_chunks <= CHUNKSIZE_MAX
            && bytes_chunks >= CHUNKSIZE_MIN)
        {
            std::cout << "  -> close enough to target size" << std::endl;
            break;
        }
        // else: not yet close enough

        // Calculate the dimension this iteration belongs to
        auto dim = i % rank;

        // Adjust the chunksize towards the target size
        if (bytes_chunks < bytes_target) {
            // If high dimensions should be favoured, change the dim to work on
            if (larger_high_dims) {
                dim = (rank - 1) - dim;
            }

            // Multiply by two
            std::cout << "  -> increasing size of dim " << dim << std::endl;
            chunks[dim] = chunks[dim] * 2;
        }
        else {
            if (larger_high_dims && rank > 1) {
                // Stay on low dimensions one step longer // TODO generalise!
                if (dim > 0) {
                    dim--;
                }

                // Skip the reduction if this is the last dim and it should
                // not be reduced
                if (dim == rank - 1) {
                    std::cout << "  -> dim " << dim << " is highest; "
                              << "skipping reduction" << std::endl;
                    continue;
                }
            }

            // Divide the chunk size of the current dim by two
            std::cout << "  -> reducing size of dim " << dim << std::endl;
            chunks[dim] = 1 + ((chunks[dim] - 1) / 2);  // ceiling!
            // NOTE integer division fun; can do this because all are unsigned
            // and the chunks entry is always nonzero
        }
    }

    return;
}


/// Optimizes the chunks using the max_extend information, favouring last dims
template<typename Cont, typename IdxCont=std::vector<unsigned short>>
void __opt_chunks_with_max_extend(Cont &chunks,
                                  const Cont &max_extend,
                                  const hsize_t typesize,
                                  const unsigned int CHUNKSIZE_MAX,
                                  const bool opt_inf_dims,
                                  const bool larger_high_dims)
{
    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
    };


    // -- Parse dims and prepare algorithm -- //

    // Create a container with the available dims indices
    IdxCont dims(chunks.size());
    std::iota(dims.begin(), dims.end(), 0);

    // Determine the finite dims
    auto dims_fin = __find_all_idcs(max_extend,
                                    [](auto extd){return extd != 0;});
    // Ideally, an integer multiple of the chunk size along this dim should
    // be equal to the maximum extend

    // Determine the infinite dims
    auto dims_inf = __find_all_idcs(max_extend,
                                    [](auto extd){return extd == 0;});
    // As the final extend along these dims is not known, we can not make a
    // good guess for these. Instead, we should use the leverage we have for
    // optimizing the chunk size along the finite dims. The infinite dims will
    // thus, most likely, end up with shorter chunk sizes.

    // Among the finite dims, determine the dims that can still be filled,
    // i.e. those where the chunk size does not reach the max_extend
    IdxCont dims_fillable;
    for (auto dim: dims_fin) {
        if (max_extend[dim] > chunks[dim]) {
            dims_fillable.push_back(dim);
        }
    }

    // Check if to reverse index containers to favour higher dims
    if (larger_high_dims) {
        // Reverse all index containers
        std::reverse(dims_fillable.begin(), dims_fillable.end());
        std::reverse(dims_fin.begin(), dims_fin.end());
        std::reverse(dims_inf.begin(), dims_inf.end());

        // NOTE do not actually _need_ to reverse the finite dims container,
        // doing it only for consistency.
    }


    // -- Optimization of finite (and still fillable) dims -- //

    if (!dims_fillable.size()) {
        std::cout << "  no finite dims available to optimize" << std::endl;
    }
    else {
        std::cout << "  => optimizing " << dims_fillable.size()
                  << " finite dim(s) where max_extend is not yet reached ..."
                  << std::endl;

        // Loop over the fillable dims indices
        for (auto dim: dims_fillable) {
            // Check if there is still potential for optimization
            // NOTE this could be more thorough
            if (bytes(chunks) == CHUNKSIZE_MAX) {
                std::cout << "    reached maximum chunk size" << std::endl;
                break;
            }

            // Check if the max_extend is an integer multiple of the chunksize
            if (max_extend[dim] % chunks[dim] == 0) {
                // Find the divisor
                size_t factor = max_extend[dim] / chunks[dim];

                // It might fit in completely ...
                if (factor * bytes(chunks) <= CHUNKSIZE_MAX) {
                    // It does. Adjust chunks and continue with next dim
                    std::cout << "    dim " << dim << " can be filled "
                              << "completely. Factor: " << factor << std::endl;
                    chunks[dim] = chunks[dim] * factor;
                    continue;
                }
                // else: would not fit in completely

                // Starting from the largest possible factor, find the largest
                // integer divisor of the original factor, i.e.: one that will
                // also completely cover the max_extend
                for (size_t div=(CHUNKSIZE_MAX / bytes(chunks));
                     div >= 1; div--) {
                    // Check if it is an integer divisor
                    if (factor % div == 0) {
                        // Yes! The _new_ factor is now this value
                        factor = div;
                        break;
                    }
                }
                // NOTE Covers the edge case of max. factor == 1: the loop will
                // perform only one iteration and resulting factor will be 1,
                // leading (effectively) to no scaling.

                // Scale the chunksize with this factor
                if (factor > 1) {
                    std::cout << "    dim " << dim << ": scaling with factor "
                              << factor << " ..." << std::endl;

                    chunks[dim] = chunks[dim] * factor;
                }
            }
            else {
                // Not divisible. Check if the max_extend could be reached w/o
                // exceeding the max chunksize
                double factor = (float) max_extend[dim] / (float) chunks[dim];

                if (factor * bytes(chunks) <= CHUNKSIZE_MAX) {
                    // Yep. Just extend this dimension to the max_extend, done.
                    std::cout << "    dim " << dim << " can be filled "
                              << "completely. "
                              << "(difference: " << max_extend[dim]-chunks[dim]
                              << ", factor: " << factor << ")" << std::endl;
                    chunks[dim] = max_extend[dim];
                }
                else {
                    // Cannot further extend.
                    std::cout << "    dim " << dim << " cannot be extended to "
                              << "fill max_extend without exceeding maximum "
                              << "chunksize! "
                              << "(difference: " << max_extend[dim]-chunks[dim]
                              << ", factor: " << factor << ")" << std::endl;
                }
            }
            // Done with this index
        }
    }


    // -- Optimization of infinite dims -- //

    if (!opt_inf_dims) {
        std::cout << "  optimization of infinite dims disabled" << std::endl;
    }
    else if (!dims_inf.size()) {
        std::cout << "  no infinite dims available to optimize" << std::endl;
    }
    else if (bytes(chunks) == CHUNKSIZE_MAX) {
        std::cout << "  cannot further optimize using infinite dims"
                  << std::endl;
    }
    else {
        std::cout << "  => optimizing " << dims_inf.size()
                  << " infinite dim(s) to fill max chunksize ..." << std::endl;
        
        // Loop over indices of inf. dims
        // NOTE Depending on the chunk sizes, this might only have an effect
        //      on the first index considered ... but that's fine for now.
        for (auto dim: dims_inf) {            
            // Calculate the factor to make the chunk as big as possible
            size_t factor = CHUNKSIZE_MAX / bytes(chunks); // rounding down

            // If large enough, scale it by that factor
            if (factor > 1) {
                std::cout << "    dim " << dim << ": scaling with factor "
                          << factor << " ..." << std::endl;

                chunks[dim] = chunks[dim] * factor;
            }
        }

    }


    // -- Done. -- //
    // Check if everything went fine (only a safeguard ...)
    if (bytes(chunks) > CHUNKSIZE_MAX) {
        throw std::runtime_error("Calculated chunks exceed CHUNKSIZE_MAX! "
                                 "This should not have happened!");
    }

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
 *                          the chunk size is increased along the open dims to
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
// TODO update doxygen; make sure it's clear that max_extend opt is only
//      used when max_extend is given -- but then optimizes infinite axes!
// TODO is it reasonable to use const here?
template<typename Cont=std::vector<hsize_t>>
const Cont guess_chunksize(const hsize_t typesize,
                           const Cont io_extend,
                           Cont max_extend = {},
                           const bool avoid_low_chunksize = true,
                           const bool opt_inf_dims = true,
                           const bool larger_high_dims = true,
                           const unsigned int CHUNKSIZE_MAX = 1048576,  // 1M
                           const unsigned int CHUNKSIZE_MIN = 8192,     // 8k
                           const unsigned int CHUNKSIZE_BASE = 262144)  // 256k
{
    // -- Initialisation -- //

    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
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


    // -- Check correctness of arguments and extract some info -- //
    // Get the rank
    auto rank = io_extend.size();

    // For scalar datasets, chunking is not available
    if (rank == 0) {
        throw std::invalid_argument("Cannot guess chunksize for scalar "
                                    "dataset!");
    }

    // Check if io_extend has no illegal values (<=0) // TODO valid assumption?
    for (auto val: io_extend) {
        if (val <= 0) {
            throw std::invalid_argument("Argument 'io_extend' contained "
                                        "illegal (zero or negative) value(s)! "
                                        "io_extend: " + vec2str(io_extend));
        }
    }

    // Find out if the max_extend is given and determine whether dset is finite
    bool dset_finite;
    bool all_dims_inf;

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
                       == max_extend.end()); // i.e., no "0" found

        // Or even all are infinitely long
        all_dims_inf = true;
        for (auto l: max_extend) {
            if (l != 0) {
                all_dims_inf = false;
                break;
            }
        }
        
        // TODO Check if H5S_UNLIMITED could be a valid value in max_extend?
    }
    else {
        // Have to assume the max_extend is infinite in all directions
        dset_finite = false;
        all_dims_inf = true;
    }


    std::cout << "Guessing appropriate chunk size using:" << std::endl;
    std::cout << "  io_extend:         " << vec2str(io_extend) << std::endl;
    std::cout << "  max_extend:        " << vec2str(max_extend) << std::endl;
    std::cout << "  rank:              " << rank << std::endl;
    std::cout << "  finite dset?       " << dset_finite << std::endl;
    std::cout << "  all dims infinite? " << all_dims_inf << std::endl;
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
        return Cont(rank, 1);
    }


    // For a finite dataset, that would fit into CHUNKSIZE_MAX when maximally
    // extended can only have (and only need!) a single chunk
    if (dset_finite
        && max_extend.size()
        && (bytes(max_extend) <= CHUNKSIZE_MAX))
    {
        std::cout << "  -> maximally extended dataset will fit into one chunk"
                  << std::endl;
        return Cont(max_extend);
    }


    // -- Optimize for one I/O operation fitting into chunk -- //

    // Create the temporary container that will store the chunksize values.
    // It starts with a copy of the extend values for I/O operations.
    Cont _chunks(io_extend);

    // Determine the size (in bytes) of a write operation with this extend
    auto bytes_io = bytes(io_extend);
    std::cout << "  I/O op. size:      " << bytes_io
              << " B  (" << bytes_io/1024 << " kiB) " << std::endl;

    // Determine if an I/O operation fits into a single chunk
    bool fits_into_chunk = (bytes_io <= CHUNKSIZE_MAX);
    std::cout << "  fits into chunk?   " << fits_into_chunk << std::endl;

    // Do we need to optimize?
    if (!fits_into_chunk) {
        // The I/O operation does not fit into a chunk
        // Aim to fit the I/O operation into the chunk -> target: max chunksize
        std::cout << "  trying to use the fewest possible chunks for a single "
                     "I/O operation ..." << std::endl;

        __opt_chunks_target(_chunks, CHUNKSIZE_MAX, // <- target value
                            typesize, CHUNKSIZE_MAX, CHUNKSIZE_MIN,
                            larger_high_dims);
        // NOTE this relies on _chunks == io_extend
    }
    else if (   all_dims_inf && avoid_low_chunksize
             && bytes(_chunks) < CHUNKSIZE_BASE)
    {
        // The I/O operation _does_ fit into a chunk, but the dataset is
        // infinite in _all directions_ and small chunksizes can be very
        // inefficient -> optimize towards some base value
        std::cout << "  enlarging chunksize to be closer to base chunksize ..."
                  << std::endl;

        __opt_chunks_target(_chunks, CHUNKSIZE_BASE, // <- target value
                            typesize, CHUNKSIZE_MAX, CHUNKSIZE_MIN,
                            larger_high_dims);
    }
    // else: no other optimization towards a target size make sense


    // -- If given, take the max_extend into account -- //

    // This is only possible if the current chunk size is not already above the
    // upper limit, CHUNKSIZE_MAX
    if (max_extend.size() && (bytes(_chunks) < CHUNKSIZE_MAX)) {
        std::cout << "  can (potentially) optimize using max_extend info ..."
                  << std::endl;

        __opt_chunks_with_max_extend(_chunks, max_extend,
                                     typesize, CHUNKSIZE_MAX,
                                     opt_inf_dims, larger_high_dims);
    }
    // else: no further optimization possible



    // -- Done. -- //

    // Create a const version of the temporary chunks vector
    const Cont chunks(_chunks);
    // TODO is this the best way to do this?

    return chunks;
}


} // namespace DataIO
} // namespace Utopia
#endif // HDFCHUNKING_HH
