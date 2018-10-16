#ifndef UTOPIA_MODELS_HDFBENCH_HH
#define UTOPIA_MODELS_HDFBENCH_HH

#include <map>
#include <vector>
#include <chrono>
#include <limits>
#include <numeric>
#include <functional>
#include <thread>

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
    typedef std::chrono::high_resolution_clock Clock;

    /// Type of a time point, retrieved from the clock
    using Time = std::chrono::high_resolution_clock::time_point;

    /// Type of the duration measure, should be a floating-point type
    typedef std::chrono::duration<double> dsec;

    /// Type of a benchmark function pointer
    using BenchFunc = std::function<const double(const std::string, Config)>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng


    // -- Members of this model -- //
    /// A map of implemented setup functions for datasets
    std::map<std::string, BenchFunc> _setup_funcs;

    /// A map of implemented write functions
    std::map<std::string, BenchFunc> _write_funcs;
    
    /// Configuration for the benchmarks
    const std::map<std::string, Config> _benchmarks;

    /// The results of the measurements, stored under the benchmark name
    std::map<std::string, double> _times;

    
    // -- Datasets -- //
    /// Dataset to store the write times in
    std::shared_ptr<DataSet> _dset_times;

    /// Dataset to write test data to are stored in a map of dataset pointers
    std::map<std::string, std::shared_ptr<DataSet>> _dsets;


    // -- Configuration parameters applicable to all benchmarks -- //
    /// Whether to perform a write operation at time step 0
    const bool _initial_write;
    
    /// Whether to delete datasets after the last step
    const bool _delete_afterwards;
    
    /// Sleep time in seconds at the beginning of each step
    const std::chrono::duration<double> _sleep_step;
    
    /// Sleep time in seconds before each benchmark
    const std::chrono::duration<double> _sleep_bench;



    // -- Construction helper functions -- //
    /// Parse the benchmarks into a map of configurations
    std::map<std::string, Config> parse_benchmarks() {
        auto benchmarks = as_vector<std::string>(this->_cfg["benchmarks"]);
        std::map<std::string, Config> cfg;

        for (auto &bname : benchmarks) {
            cfg[bname] = as_<Config>(this->_cfg[bname]);
        }
        return cfg;
    }

public:
    /// Construct the HdfBench model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    HdfBenchModel (const std::string name,
                   ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Set maps for setup and write functions
        _setup_funcs(),
        _write_funcs(),

        // Get the set of enabled benchmarks from the config
        _benchmarks(parse_benchmarks()),

        // Create the temporary map for measured times and the times dataset
        _times(),
        _dset_times(this->_hdfgrp->open_dataset("times")),
        _dsets(),

        // Extract config parameters applicable to all benchmarks
        _initial_write(as_bool(this->_cfg["initial_write"])),
        _delete_afterwards(as_bool(this->_cfg["delete_afterwards"])),
        _sleep_step(as_double(this->_cfg["sleep_step"])),
        _sleep_bench(as_double(this->_cfg["sleep_bench"]))
    {   
        // Check arguments
        if (_delete_afterwards) {
            throw std::invalid_argument("delete_afterwards feature is not yet "
                                        "implemented!");
        }


        // Set up the function mappings . . . . . . . . . . . . . . . . . . . .
        // FIXME Creating func maps should be possible in initializer list!

        this->_log->debug("Associating setup functions ...");
        _setup_funcs["setup_2d"] = setup_2d;
        

        this->_log->debug("Associating write functions ...");
        _write_funcs["write_const"] = write_const;

        
        this->_log->debug("Associated {} setup and {} write function(s).",
                          _setup_funcs.size(), _write_funcs.size());


        // Carry out the setup benchmark  . . . . . . . . . . . . . . . . . . .
        this->_log->info("Performing setup and initial benchmark of {} "
                         "configuration(s) ...", _benchmarks.size());
        this->_log->debug("initial_write: {},  sleep_step: {}s,  "
                          "sleep_bench: {}s", _initial_write ? "yes" : "no",
                          _sleep_step.count(), _sleep_bench.count());

        // Use the below iteration also to create a vector of benchmark names
        // that correspond to the coordinates of the "benchmark" dimension of
        // the _times dataset.
        std::vector<std::string> benchmark_names;

        for (auto const& [bname, bcfg] : _benchmarks) {
            // Setup the dataset and store the time needed
            _times[bname] = this->benchmark<true>(bname, bcfg);

            // Perform one write operation, if configured to do so, and add
            // the time on top
            if (_initial_write) {
                _times[bname] += this->benchmark(bname, bcfg);
            }

            // Add the name to the vector of benchmark names
            benchmark_names.push_back(bname);
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
                                                               {"t",
                                                                "benchmark"});
        _dset_times->add_attribute("coords_benchmark", benchmark_names);


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
        for (auto const& [bname, bcfg] : _benchmarks) {
            std::this_thread::sleep_for(_sleep_bench);

            _times[bname] = this->benchmark(bname, bcfg);
        }
    }


    /// Write data
    void write_data () {   
        _dset_times->write(_times.begin(), _times.end(),
                           [](auto& result) { return result.second; });
    }


protected:

    // Helper functions .......................................................


    /// Carries out the benchmark function associated with the given name
    template<bool setup=false>
    const double benchmark(const std::string &bname, const Config &bcfg) {
        // Get the name of the setup/benchmark function, then resolve it
        BenchFunc bfunc;
        if constexpr (setup) {
            const auto func_name = as_str(bcfg["setup_func"]);
            bfunc = _setup_funcs.at(func_name);
        }
        else {
            const auto func_name = as_str(bcfg["write_func"]);
            bfunc = _write_funcs.at(func_name);
        }

        // Call the function; its return value is the time it took to execute
        const auto btime = bfunc(bname, bcfg);

        // Log the time, then return it        
        this->_log->debug("Benchmark result {:>13s} {} : {:>10.3f} ms",
                          bname, setup ? "setup" : "write", btime * 1E3);
        return btime; 
    }

    /// Returns the time (in seconds) since the given time point
    const double time_since(const Time start) {
        return time_between(start, Clock::now());
    }
    
    /// Returns the absolute time (in seconds) between the given time points
    const double time_between(const Time start, const Time end) {
        dsec seconds = abs(end - start);
        return seconds.count();
    }


    // Setup functions ........................................................

    /// Sets up a 2d dataset with a given number of columns
    BenchFunc setup_2d = [this](auto bname, auto cfg){
        // Without measuring time, extract the parameters
        auto num_cols = as_<std::size_t>(cfg["num_cols"]);
        auto num_rows = this->get_time_max() + 1;

        const auto start = Clock::now();
        // -- benchmark start -- //

        // Create the dataset and set its capacity
        _dsets[bname] = this->_hdfgrp->open_dataset(bname);
        _dsets[bname]->set_capacity({num_rows, num_cols});

        // --- benchmark end --- //
        return time_since(start);
    };

    // Benchmark functions ....................................................

    /// Writes a constant value into the 
    BenchFunc write_const = [this](auto bname, auto cfg){
        // Determine the value to write
        auto val = as_double(cfg["const_val"]);

        // Determine iterator length by factorizing the shape
        const auto shape = as_vector<std::size_t>(cfg["write_shape"]);
        const auto it_len = std::accumulate(shape.begin(), shape.end(),
                                            1, std::multiplies<std::size_t>());

        const auto start = Clock::now();
        // -- benchmark start -- //

        _dsets[bname]->write(boost::counting_iterator<std::size_t>(0),
                             boost::counting_iterator<std::size_t>(it_len),
                             [&val](auto &count){ return val; });

        // --- benchmark end --- //
        return time_since(start);
    };
};


} // namespace HdfBench
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_HDFBENCH_HH
