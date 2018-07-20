#ifndef HDFCHUNKING_HH
#define HDFCHUNKING_HH

#include <hdf5.h>

#include <dune/utopia/core/logging.hh>


namespace Utopia {
namespace DataIO {


// -- Helper functions -- //

/**
 * @brief   Finds all indices of elements in a vector that matches the given
 *          predicate
 *
 * @param   vec       The object to find the indices in
 * @param   pred      The predicate to determine the indices to be found
 *
 * @tparam  Cont      The container type
 * @tparam  Predicate The predicate type
 */
template<typename Cont,
         typename Predicate,
         typename IdxCont=std::vector<unsigned short>>
IdxCont find_all_idcs (Cont &vec, Predicate pred) {
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

/**
 * @brief   Optimizes the chunks along all axes to find a good default
 * @detail  This algorithm is only aware of the current size of the chunks and
 *          the target byte size of a chunk. Given that information, it either
 *          tries to reduce the extend of chunk dimensions, or enlarge it. To
 *          do that, it iterates over all chunk dimensions and either doubles
 *          the extend or halves it. Once within 50% of the target byte size,
 *          the algorithm stops.
 *          Also, it takes care to remain within the bounds of CHUNKSIZE_MAX
 *          and CHUNKSIZE_MIN. If a target byte size outside of these bounds is
 *          given, it will adjust it accordingly. For a typesize larger than
 *          CHUNKSIZE_MAX, this algorithm cannot perform any reasonable actions
 *          and will throw an exception; this case should be handled outside of
 *          this function!
 *
 * @param   chunks           The current chunk values that are to be optimized
 * @param   bytes_target     Which byte size to optimize the chunks to
 * @param   typesize         The byte size of a single entry, needed to
 *                           calculate the total bytesize of a whole chunk
 * @param   CHUNKSIZE_MAX    The maximum allowed bytesize of a chunk
 * @param   CHUNKSIZE_MIN    The minimum allowed bytesize of a chunk
 * @param   larger_high_dims If true, dimensions with high indices will be
 *                           favoured for enlarging chunk extend in that dim
 * @param   log              The logger object to use
 */
template<typename Cont, typename Logger>
void opt_chunks_target(Cont &chunks,
                       double bytes_target,
                       const hsize_t typesize,
                       const unsigned int CHUNKSIZE_MAX,
                       const unsigned int CHUNKSIZE_MIN,
                       const bool larger_high_dims,
                       Logger log)
{
    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
    };

    // Check the case of typesize larger than CHUNKSIZE_MAX; cannot do anything
    // in that case -> safer to throw an exception.
    if (typesize > CHUNKSIZE_MAX) {
        throw std::invalid_argument("Cannot use opt_chunks_target with a "
                                    "typesize larger than CHUNKSIZE_MAX!");
    }

    log->debug("Starting optimization towards target size:"
               "  {:7d}B  ({:.1f} kiB)", bytes_target, bytes_target/1024);

    // Ensure the target chunk size is between CHUNKSIZE_MIN and CHUNKSIZE_MAX
    // in order to not choose too large or too small chunks
    if (bytes_target > CHUNKSIZE_MAX) {
        bytes_target = CHUNKSIZE_MAX;

        log->debug("Target size too large! New target size:"
                   "  {:7d}B  ({:.1f} kiB)", bytes_target, bytes_target/1024);
    }
    else if (bytes_target < CHUNKSIZE_MIN) {
        bytes_target = CHUNKSIZE_MIN;

        log->debug("Target size too small! New target size:"
                   "  {:7d}B  ({:.1f} kiB)", bytes_target, bytes_target/1024);
    }

    // ... and a variable that will store the size (in bytes) of this specific
    // chunk configuration
    std::size_t bytes_chunks;

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
        
        log->debug("Chunk size:"
                   "  {:7d}B  ({:.1f} kiB)", bytes_chunks, bytes_chunks/1024);

        // If close enough to target size, optimization is finished
        if (   (std::abs(bytes_chunks - bytes_target) / bytes_target < 0.5)
            && bytes_chunks <= CHUNKSIZE_MAX
            && bytes_chunks >= CHUNKSIZE_MIN)
        {
            log->debug("Close enough to target size now.");
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
            log->debug("Doubling extend of chunk dimension {} ...", dim);
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
                    log->debug("Skipping reduction of chunk dimension {}, "
                               "because it is the highest ...", dim);
                    continue;
                }
            }

            // Divide the chunk size of the current dim by two
            log->debug("Halving extend of chunk dimension {} ...", dim);
            chunks[dim] = 1 + ((chunks[dim] - 1) / 2);  // ceiling!
            // NOTE integer division fun; can do this because all are unsigned
            //      and the chunks entry is always nonzero
        }
    }

    return;
}


/**
 * @brief   Optimize chunk sizes using max_extend information
 * @detail  This algorithm is aware of the maximum extend of a dataset and can
 *          use that information during optimization, aiming to increase the
 *          size of the chunks towards CHUNKSIZE_MAX as far as possible without
 *          going beyond max_extend. The paradigm here is that the _number_ of
 *          chunks needed for read/write operations should be minimized while
 *          trying to keep a chunk's byte size below a certain value.
 *          The algorithm distinguishes between dimensions that have a finite
 *          extend and those that can grow to H5S_UNLIMITED, i.e. "infinite"
 *          extend.
 *          First, the aim is to try to cover the max_extend in the finite
 *          dimensions. It checks if an integer multiple is needed to reach the
 *          maximum extend.
 *          If, after that, the target CHUNKSIZE_MAX is not yet reached and
 *          the opt_inf_dims flag is set, the chunk sizes in the unlimited
 *          dimensions are extended as far as possible, assuming that they
 *          were chosen unlimited because they _will_ be filled at some point
 *          and larger chunk sizes will reduce the _number_ of chunks needed
 *          during read/write operations.
 *          
 * @param   chunks           The current chunk values that are to be optimized
 * @param   max_extend       The maximum extend of the dataset
 * @param   typesize         The byte size of a single entry, needed to
 *                           calculate the total bytesize of a whole chunk
 * @param   CHUNKSIZE_MAX    The maximum allowed bytesize of a chunk
 * @param   opt_inf_dims     Whether to optimize the infinite dimensions or not
 * @param   larger_high_dims If true, dimensions with high indices will be
 *                           favoured for enlarging chunk extend in that dim
 * @param   log              The logger object to use
 */
template<typename Cont,
         typename Logger,
         typename IdxCont=std::vector<unsigned short>>
void opt_chunks_with_max_extend(Cont &chunks,
                                const Cont &max_extend,
                                const hsize_t typesize,
                                const unsigned int CHUNKSIZE_MAX,
                                const bool opt_inf_dims,
                                const bool larger_high_dims,
                                Logger log)
{
    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
    };

    // Check the case of typesize larger than CHUNKSIZE_MAX; cannot do anything
    // in that case -> safer to throw an exception.
    if (typesize > CHUNKSIZE_MAX) {
        throw std::invalid_argument("Cannot use opt_chunks_with_max_extend "
                                    "with a typesize larger than "
                                    "CHUNKSIZE_MAX!");
    }

    // -- Parse dims and prepare algorithm -- //

    // Create a container with the available dims indices
    IdxCont dims(chunks.size());
    std::iota(dims.begin(), dims.end(), 0);

    // Determine the finite dims
    auto dims_fin = find_all_idcs(max_extend,
                                  [](auto l){return l != H5S_UNLIMITED;});
    // Ideally, an integer multiple of the chunk size along this dim should
    // be equal to the maximum extend

    // Determine the infinite dims
    auto dims_inf = find_all_idcs(max_extend,
                                  [](auto l){return l == H5S_UNLIMITED;});
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
        log->debug("No finite dimensions available to optimize.");
    }
    else {
        log->debug("Optimizing {} finite dimension(s) where max_extend is not "
                   "yet reached ...", dims_fillable.size());

        // Loop over the fillable dims indices
        for (auto dim: dims_fillable) {
            // Check if there is still potential for optimization
            // NOTE this could be more thorough
            if (bytes(chunks) == CHUNKSIZE_MAX) {
                log->debug("Reached maximum chunksize.");
                break;
            }

            // Check if the max_extend is an integer multiple of the chunksize
            if (max_extend[dim] % chunks[dim] == 0) {
                // Find the divisor
                size_t factor = max_extend[dim] / chunks[dim];

                // It might fit in completely ...
                if (factor * bytes(chunks) <= CHUNKSIZE_MAX) {
                    // It does. Adjust chunks and continue with next dim
                    log->debug("Dimension {} can be filled completely. "
                               "Factor: {}", dim, factor);
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
                    log->debug("Scaling dimension {} with factor {} ...",
                               dim, factor);

                    chunks[dim] = chunks[dim] * factor;
                }
            }
            else {
                // Not divisible. Check if the max_extend could be reached w/o
                // exceeding the max chunksize
                double factor = (float) max_extend[dim] / (float) chunks[dim];

                if (factor * bytes(chunks) <= CHUNKSIZE_MAX) {
                    // Yep. Just extend this dimension to the max_extend, done.
                    log->debug("Dimension {} can be filled completely. "
                               "(difference: {:.2e}, factor: {:.2e})",
                               dim, max_extend[dim]-chunks[dim], factor);

                    chunks[dim] = max_extend[dim];
                }
                else {
                    // Cannot further extend.
                    log->debug("Dimension {} cannot be extended to fill "
                               "max_extend without exceeding maximum "
                               "chunksize! "
                               "(difference: {:.2e}, factor: {:.2e})",
                               dim, max_extend[dim]-chunks[dim], factor);
                }
            }
            // Done with this index
        }
    }


    // -- Optimization of infinite dims -- //

    if (!opt_inf_dims) {
        log->debug("Optimization of unlimited dimensions is disabled.");
    }
    else if (!dims_inf.size()) {
        log->debug("No unlimited dimensions available to optimize.");
    }
    else if (bytes(chunks) == CHUNKSIZE_MAX) {
        log->debug("Cannot further optimize using unlimited dimensions.");
    }
    else {
        log->debug("Optimizing {} unlimited dimension(s) to fill the maximum "
                   "chunk size ...", dims_inf.size());
        
        // Loop over indices of inf. dims
        // NOTE Depending on the chunk sizes, this might only have an effect
        //      on the first index considered ... but that's fine for now.
        for (auto dim: dims_inf) {            
            // Calculate the factor to make the chunk as big as possible
            size_t factor = CHUNKSIZE_MAX / bytes(chunks); // rounding down

            // If large enough, scale it by that factor
            if (factor > 1) {
                log->debug("Scaling dimension {} with factor {} ...",
                           dim, factor);

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


// -- The actual guess_chunksize method, publicly used -- //

/**  \page opt_chunksize Algorithms for optimizing chunk size
 *
 * \section idea General idea
 * The general idea of these algorithms is that in order for I/O operations to
 * be fast, a reasonable chunk size needs to be given. Given the information
 * known about the data to be written, an algorithm should automatically
 * determine an optimal size for the chunks.
 * What is optimal in the case of HDF5? Two main factors determine the speed
 * of I/O operations in HDF5: the number of chunk lookups necessary and the
 * size of the chunks. If either of the two is too large, performance suffers.
 * To that end, these algorithms try to make the chunks as large as possible
 * while staying below an upper limit, CHUNKSIZE_MAX, which -- per default --
 * corresponds to the default size of the HDF5 chunk cache.
 *
 * Note that the algorithms prioritize single I/O operations, such that writing
 * is easy. Depending on the shape of your data and how you want to _read_ it,
 * this might not be ideal. For those cases, it might be more reasonable to
 * specify the chunk sizes manually.
 *
 *
 * \section implementation Implementation
 * The implementation is done via a main handler method, `guess_chunksize`
 * and two helper methods, which implement the algorithms.
 * The main method checks arguments and determines which algorithms can and
 * need be applied. The helper methods then carry out the optimization, working
 * on a common `chunks` container. 
 */ 


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
 *                          as possible. If _not_ given, the max_extend will
 *                          be assumed to be the same as the io_extend.
 * @param   opt_inf_dims    Whether to optimize unlimited dimensions or not. If
 *                          set, and there is still room left to optimize after
 *                          the finite dimensions have been extended, the
 *                          chunks in the unlimited dimensions are extended as
 *                          far as possible.
 * @param   larger_high_dims If set, dimensions with higher indices are
 *                          favourable enlarged and less favourable reduced.
 *                          This can be useful if it is desired to keep these
 *                          dimensions together, e.g. because they are written
 *                          close to each other (e.g., as inner part of a loop)
 * @param   CHUNKSIZE_MAX   Largest chunksize; should not exceed 1MiB too much,
 *                          or, more precisely: should fit into the chunk cache
 *                          which (by default) is 1MiB large 
 * @param   CHUNKSIZE_MIN   smallest chunksize; should be above a few KiB
 * @param   CHUNKSIZE_BASE  base factor for the target chunksize (in bytes) if
 *                          the max_extend is unlimited in all dimensions and
 *                          opt_inf_dims is activated. This value is not used
 *                          in any other scenario.
 *
 * @tparam  Cont            The type of the container holding the io_extend,
 *                          max_extend, and the returned chunks. If none is
 *                          given, defaults to the largest possible, i.e. a
 *                          std::vector of hsize_t elements.
 */
// NOTE Could include compression level here in the future
template<typename Cont=std::vector<hsize_t>>
const Cont calc_chunksize(const hsize_t typesize,
                          const Cont io_extend,
                          Cont max_extend = {},
                          const bool opt_inf_dims = true,
                          const bool larger_high_dims = true,
                          const unsigned int CHUNKSIZE_MAX = 1048576,  // 1M
                          const unsigned int CHUNKSIZE_MIN = 8192,     // 8k
                          const unsigned int CHUNKSIZE_BASE = 262144)  // 256k
{
    // -- Helpers -- //

    // Helper lambda for calculating bytesize of a chunks configuration
    auto bytes = [&typesize](Cont c) {
        return typesize * std::accumulate(c.begin(), c.end(),
                                          1, std::multiplies<>());
    };

    // Helper lambda for string representation of vectors
    auto vec2str = [](const std::vector<hsize_t> vec) {
        std::stringstream s;
        s << "{ ";
        for (auto &extd: vec) {
            if (extd < H5S_UNLIMITED) {
                s << extd << " ";
            }
            else {
                s << "∞ ";
            }
        }
        s << "}";
        return s.str();
    };


    // -- Logging -- //
    // Get a logger to use here; note that it needs to have been set up outside
    // of here beforehand!
    auto log = spdlog::get("data_io");


    // -- Check correctness of arguments and extract some info -- //
    // Get the rank
    unsigned short rank = io_extend.size();

    // For scalar datasets, chunking is not available
    if (rank == 0) {
        throw std::invalid_argument("Cannot guess chunksize for scalar "
                                    "dataset!");
    }

    // Check if io_extend has no illegal values (<=0)
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

    if (max_extend.size()) { // Yes, was given
        // Need to check that the max_extend values are ok
        // Check that it matches the rank
        if (max_extend.size() != rank) {
            throw std::invalid_argument("Argument 'max_extend' does not have "
                                        "the same dimensionality as the rank "
                                        "of this dataset (as extracted from "
                                        "the 'io_extend' argument).");
        }

        // And that all values are valid, i.e. larger than corresp. io_extend
        for (unsigned short i=0; i < rank; i++) {
            if (max_extend[i] < io_extend[i]) {
                throw std::invalid_argument("Index " + std::to_string(i)
                                            + " of argument 'max_extend' was "
                                            "smaller than the corresponding "
                                            "io_extend value! "
                                            + std::to_string(max_extend[i])
                                            + " < "
                                            + std::to_string(io_extend[i]));
            }
        }
        // max_extend content is valid now

        // Now extract information on the properties of max_extend
        // Need to check whether any dataset dimension can be infinitely long
        dset_finite = (std::find(max_extend.begin(), max_extend.end(),
                                 H5S_UNLIMITED)
                       == max_extend.end()); // i.e., H5S_UNLIMITED _not_ found

        // Or even all are infinitely long
        all_dims_inf = true;
        for (auto ext: max_extend) {
            if (ext < H5S_UNLIMITED) {
                // This one is not infinite
                all_dims_inf = false;
                break;
            }
        }
    }
    else { // max_extend not given
        // Have to assume the max_extend is the same as the io_extend
        // Thus, the properties are known:
        dset_finite = true;
        all_dims_inf = false;

        // Set the values to those of io_extend
        max_extend.insert(max_extend.begin(),
                          io_extend.begin(), io_extend.end());
    }
    // NOTE max_extend is now a vector of same rank as io_extend


    log->info("Calculating optimal chunk size for io_extend {} and "
              "max_extend {} ...", vec2str(io_extend), vec2str(max_extend));
    log->debug("rank:                {}", rank);
    log->debug("finite dset?         {}", dset_finite);
    log->debug("all dims infinite?   {}", all_dims_inf);
    log->debug("optimize inf dims?   {}", opt_inf_dims);
    log->debug("larger high dims?    {}", larger_high_dims);
    log->debug("typesize:            {}", typesize);
    log->debug("max. chunksize:      {:7d} ({:.1f} kiB)",
               CHUNKSIZE_MAX, CHUNKSIZE_MAX/1024);
    log->debug("min. chunksize:      {:7d} ({:.1f} kiB)",
               CHUNKSIZE_MIN, CHUNKSIZE_MIN/1024);
    log->debug("base chunksize:      {:7d} ({:.1f} kiB)",
               CHUNKSIZE_BASE, CHUNKSIZE_BASE/1024);


    // -- For the simple cases, evaluate the chunksize directly -- //

    // For large typesizes, each chunk can at most contain a single element.
    // Chunks that extend to more than one element require a typesize smaller
    // than half the maximum chunksize.
    if (typesize > CHUNKSIZE_MAX / 2) {
        log->debug("Type size >= 1/2 max. chunksize -> Each cell needs to be "
                   "its own chunk.");
        return Cont(rank, 1);
    }

    // For a finite dataset, that would fit into CHUNKSIZE_MAX when maximally
    // extended, we can only have (and only need!) a single chunk
    if (dset_finite && (bytes(max_extend) <= CHUNKSIZE_MAX)) {
        log->debug("Maximally extended dataset fits will fit into single "
                   "chunk.");
        return Cont(max_extend);
    }


    // -- Step 1: Optimize for one I/O operation fitting into chunk -- //
    log->debug("Cannot apply simple optimizations. Try to fit single I/O "
               "operation into a chunk ...");

    // Create the temporary container that will store the chunksize values.
    // It starts with a copy of the extend values for I/O operations.
    Cont _chunks(io_extend);

    // Determine the size (in bytes) of a write operation with this extend
    auto bytes_io = bytes(io_extend);
    log->debug("I/O operation size:  {:7d} ({:.1f} kiB)",
               bytes_io, bytes_io/1024);


    // Determine if an I/O operation fits into a single chunk, then decide on
    // how to optimize accordingly
    if (bytes_io > CHUNKSIZE_MAX) {
        // The I/O operation does _not_ fit into a chunk
        // Aim to fit the I/O operation into the chunk -> target: max chunksize
        log->debug("Single I/O operation does not fit into chunk.");
        log->debug("Trying to use the fewest possible chunks for a single "
                   "I/O operation ...");

        opt_chunks_target(_chunks, CHUNKSIZE_MAX, // <- target value
                          typesize, CHUNKSIZE_MAX, CHUNKSIZE_MIN,
                          larger_high_dims, log);
        // NOTE The algorithm is also able to _increase_ the chunk size in
        //      certain dimensions. However, with _chunks == io_extend and the
        //      knowledge that the current bytesize of _chunks is above the
        //      maximum size, the chunk extensions will only be _reduced_.
    }
    else if (   all_dims_inf && opt_inf_dims
             && bytes(_chunks) < CHUNKSIZE_BASE) {
        // The I/O operation _does_ fit into a chunk, but the dataset is
        // infinite in _all directions_ and small chunksizes can be very
        // inefficient -> optimize towards some base value
        log->debug("Single I/O operation does fit into chunk.");
        log->debug("Optimizing chunks in unlimited dimensions to be closer "
                   "to base chunksize ...");

        opt_chunks_target(_chunks, CHUNKSIZE_BASE, // <- target value
                          typesize, CHUNKSIZE_MAX, CHUNKSIZE_MIN,
                          larger_high_dims, log);
        // NOTE There is no issue with going beyond the maximum chunksize here
    }
    else {
        // no other optimization towards a target size make sense
        log->debug("Single I/O operation does fit into a chunk.");
    }


    // To be on the safe side: Check that _chunks did not exceed max_extend
    for (unsigned short i=0; i < rank; i++) {
        if (_chunks[i] > max_extend[i]) {
            log->warn("Optimization led to chunks larger than max_extend. "
                      "This should not have happened!");
            _chunks[i] = max_extend[i];
        }
    }



    // -- Step 2: Optimize by taking the max_extend into account -- //

    // This is only possible if the current chunk size is not already above the
    // upper limit, CHUNKSIZE_MAX, and the max_extend is not already reached.
    // Also, it should not be enabled if the optimization towards unlimited
    // dimensions was already performed
    if (   !(opt_inf_dims && all_dims_inf)
        && (_chunks != max_extend) && (bytes(_chunks) < CHUNKSIZE_MAX))
    {
        log->debug("Have max_extend information and can (potentially) use it "
                   "to optimize chunk extensions.");

        opt_chunks_with_max_extend(_chunks, max_extend,
                                   typesize, CHUNKSIZE_MAX,
                                   opt_inf_dims, larger_high_dims, log);
    }
    // else: no further optimization possible



    // -- Done. -- //

    // Create a const version of the temporary chunks vector
    const Cont chunks(_chunks);

    log->info("Optimized chunk size:  {}", vec2str(chunks));
    return chunks;
}


} // namespace DataIO
} // namespace Utopia
#endif // HDFCHUNKING_HH
