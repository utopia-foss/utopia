#ifndef UTOPIA_MODELS_HDFBENCH_HH
#define UTOPIA_MODELS_HDFBENCH_HH

#include <map>
#include <vector>
#include <chrono>
#include <limits>
#include <numeric>
#include <functional>
#include <thread>
#include <iostream>

#include <hdf5.h>
#include <boost/iterator/counting_iterator.hpp>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/types.hh>


namespace Utopia {
namespace Models {
namespace HdfBench {

/// Typehelper to define types of HdfBench model 
using HdfBenchModelTypes = ModelTypes<>;


/// The HdfBench Model
/** This model implements a benchmark of Utopia's Hdf5 writing capabilities.
 *  
 *  It does not implement a manager or a grid but focusses on benchmarking the
 *  write times, given iterable data.
 */
class HdfBenchModel:
    public Model<HdfBenchModel, HdfBenchModelTypes>
{
public:
    /// The base model type
    using Base = Model<HdfBenchModel, HdfBenchModelTypes>;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type that holds the configuration
    using Config = typename Base::Config;


    // -- Types for time handling -- //
    /// Type of clock
    using Clock = std::chrono::high_resolution_clock;

    /// Type of a time point, retrieved from the clock
    using Time = std::chrono::high_resolution_clock::time_point;

    /// Type of the duration measure, should be a floating-point type
    using DurationType = std::chrono::duration<double>;

    /// Type of a benchmark function pointer
    using BenchFunc = std::function<double(const std::string, Config)>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor


    // -- Members of this model -- //
    /// A map of implemented setup functions for datasets
    const std::map<std::string, BenchFunc> _setup_funcs;

    /// A map of implemented write functions
    const std::map<std::string, BenchFunc> _write_funcs;
    
    /// Names of benchmarks
    const std::vector<std::string> _benchmarks;

    /// Configuration for the benchmarks
    const std::map<std::string, Config> _bench_cfgs;

    /// The results of the measurements, stored under the benchmark name
    std::map<std::string, double> _times;

    
    // -- Datasets -- //
    /// Dataset to store the write times in
    std::shared_ptr<DataSet> _dset_times;

    /// Dataset to write test data to are stored in a map of dataset pointers
    std::map<std::string, std::shared_ptr<DataSet>> _dsets;


    // -- Configuration parameters applicable to all benchmarks -- //    
    /// Whether to delete datasets after the last step
    const bool _delete_afterwards;
    
    /// Sleep time in seconds at the beginning of each step
    const std::chrono::duration<double> _sleep_step;
    
    /// Sleep time in seconds before each benchmark
    const std::chrono::duration<double> _sleep_bench;



    // -- Construction helper functions -- //
    /// Load the benchmark configurations into a map
    std::map<std::string, Config> load_benchmarks() {
        this->_log->debug("Loading benchmark configurations ...");
        std::map<std::string, Config> cfg;

        for (const auto &bname : _benchmarks) {
            this->_log->trace("Loading benchmark configuration '{}' ...",
                              bname);

            try {
                cfg[bname] = as_<Config>(this->_cfg[bname]);
            }
            catch (std::exception &e) {
                std::cerr << "Could not find a benchmark configuration with "
                             "name '" << bname << "'! Make sure the given "
                             "configuration contains such an entry."
                          << std::endl << "Original error message:  ";
                throw;
            }
        }
        
        this->_log->debug("Got {} benchmark configurations.", cfg.size());
        return cfg;
    }

public:
    /// Construct the HdfBench model
    /** This model aims to allow benchmarking of the Utopia Hdf5 library in a
     *  setting that is close to the actual use case, i.e.: as means for
     *  storing model output.
     *
     *  \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    HdfBenchModel (const std::string name,
                   ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Set maps for setup and write functions
        _setup_funcs({{"setup_nd", setup_nd},
                      {"setup_nd_with_chunks", setup_nd_with_chunks}}),
        _write_funcs({{"write_const", write_const}}),

        // Get the set of enabled benchmarks from the config
        _benchmarks(as_vector<std::string>(this->_cfg["benchmarks"])),
        _bench_cfgs(load_benchmarks()),

        // Create the temporary map for measured times and the times dataset
        _times(),
        _dset_times(this->_hdfgrp->open_dataset("times")),
        _dsets(),

        // Extract config parameters applicable to all benchmarks
        _delete_afterwards(as_bool(this->_cfg["delete_afterwards"])),
        _sleep_step(as_double(this->_cfg["sleep_step"])),
        _sleep_bench(as_double(this->_cfg["sleep_bench"]))
    {   
        // Check arguments
        if (_delete_afterwards) {
            throw std::invalid_argument("delete_afterwards feature is not yet "
                                        "implemented!");
        }
        
        // Some info on how many setup and write functions are available.
        this->_log->debug("Have {} setup and {} write function(s) available.",
                          _setup_funcs.size(), _write_funcs.size());


        // Carry out the setup benchmark  . . . . . . . . . . . . . . . . . . .
        const bool initial_write = as_bool(this->_cfg["initial_write"]);
        this->_log->debug("initial_write: {},  sleep_step: {}s,  "
                          "sleep_bench: {}s", initial_write ? "yes" : "no",
                          _sleep_step.count(), _sleep_bench.count());

        this->_log->info("Performing setup and initial benchmarks ...");

        for (const auto &bname : _benchmarks) {
            // Setup the dataset and store the time needed
            _times[bname] = this->benchmark<true>(bname);

            // Perform one write operation, if configured to do so, and add
            // the time on top
            if (initial_write) {
                _times[bname] += this->benchmark(bname);
            }
        }


        // Set up the times dataset and write initial data . . . . . . . . . . 
        // Set dataset capacities for the times dataset
        this->_log->debug("Setting capacity of 'times' dataset to {} x {} ...",
                          this->get_time_max() + 1, _benchmarks.size());
        _dset_times->set_capacity({this->get_time_max() + 1,
                                   _benchmarks.size()});

        // Write out the times needed for setup
        this->write_data();

        // With the dataset open, write dimension names and coordinates to
        // the dataset attributes
        _dset_times->add_attribute<std::array<std::string, 2>>("dims",
                                                               {{"t",
                                                                "benchmark"}});
        _dset_times->add_attribute("coords_benchmark", _benchmarks);
        _dset_times->add_attribute("initial_write", initial_write);


        this->_log->debug("Finished constructing HdfBench '{}'.", this->_name);
    }

    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail The "iteration" in this model is the step that _creates_ the
     *          data that is written in the write_data method, i.e.: it carries
     *          out the benchmarks and stores the corresponding times in the
     *          _times member, to be written out in write_data
     */
    void perform_step () {
        // Sleep before the actual step is carried out
        std::this_thread::sleep_for(_sleep_step);
        // NOTE Duration might be zero, not triggering a sleep. Same below.

        // Carry out the benchmarks, optionally sleeping some time before that
        for (const auto &bname : _benchmarks) {
            std::this_thread::sleep_for(_sleep_bench);

            _times[bname] = this->benchmark(bname);
        }
    }


    /// Monitor model information
    /** @detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small. 
     */
    void monitor ()
    {
        // Can supply information to the monitor here in two ways:
        // this->_monitor.set_entry("key", value);
        // this->_monitor.set_entry("key", [this](){return 42.;});
    }


    /// Write the result times of each benchmark
    void write_data () {   
        _dset_times->write(_benchmarks.begin(), _benchmarks.end(),
                           [this](const auto& bname) {
                                return this->_times.at(bname);
                            });
    }


protected:

    // Benchmarking ...........................................................

    /// Carries out the benchmark associated with the given name
    template<bool setup=false>
    double benchmark(const std::string &bname) {
        // Get the benchmark configuration entry
        const auto bcfg = _bench_cfgs.at(bname);

        // Get the name of the setup/benchmark function, then resolve it
        BenchFunc bfunc;
        if constexpr (setup) {
            bfunc = _setup_funcs.at(as_str(bcfg["setup_func"]));
        }
        else {
            bfunc = _write_funcs.at(as_str(bcfg["write_func"]));
        }

        // Call the function; its return value is the time it took to execute
        const auto btime = bfunc(bname, bcfg);

        // Log the time, then return it        
        this->_log->debug("Benchmark result {:>20s} {} : {:>10.3f} ms",
                          bname, setup ? "setup" : "write", btime * 1E3);
        return btime; 
    }

    /// Returns the time (in seconds) since the given time point
    double time_since(const Time start) {
        return time_between(start, Clock::now());
    }
    
    /// Returns the absolute time (in seconds) between the given time points
    double time_between(const Time start, const Time end) {
        const DurationType seconds = abs(end - start);
        return seconds.count();
    }


    // Setup functions ........................................................

    /* @brief Sets up an n-dimensional dataset
     * @detail The dataset shape corresponds to the write_shape argument, but
     *         with an extra dimension in front that has as extend time_max + 1
     */
    BenchFunc setup_nd = [this](const auto& bname, auto cfg){
        // Determine the shape of the final dataset
        auto shape = as_vector<hsize_t>(cfg["write_shape"]);
        shape.insert(shape.begin(), this->get_time_max() + 1);

        const auto start = Clock::now();
        // -- benchmark start -- //

        // Create the dataset and set its capacity
        _dsets[bname] = this->_hdfgrp->open_dataset(bname);
        _dsets[bname]->set_capacity(shape);

        // --- benchmark end --- //
        return time_since(start);
    };


    BenchFunc setup_nd_with_chunks = [this](const auto& bname, auto cfg){
        // Call the regular setup_nd to set up the dataset
        const auto time_setup = this->setup_nd(bname, cfg);

        // Extract the chunks argument
        const auto chunks = as_vector<hsize_t>(cfg["chunks"]);

        const auto start = Clock::now();
        // -- benchmark start -- //

        // Set the chunks value
        _dsets[bname]->set_chunksize(chunks);

        // --- benchmark end --- //
        return time_setup + time_since(start);
    };    


    // Write functions ........................................................

    /// Writes a constant value into the dataset
    BenchFunc write_const = [this](const auto& bname, auto cfg){
        // Determine the value to write
        const auto val = as_double(cfg["const_val"]);

        // Determine iterator length by factorizing the shape
        const auto shape = as_vector<std::size_t>(cfg["write_shape"]);
        const auto it_len = std::accumulate(shape.begin(), shape.end(),
                                            1, std::multiplies<std::size_t>());

        const auto start = Clock::now();
        // -- benchmark start -- //

        // Can use the counting iterator as surrogate
        _dsets[bname]->write(boost::counting_iterator<std::size_t>(0),
                             boost::counting_iterator<std::size_t>(it_len),
                             [&val]([[maybe_unused]] auto &count){
                                return val;
                             });

        // --- benchmark end --- //
        return time_since(start);
    };
};


} // namespace HdfBench
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_HDFBENCH_HH
