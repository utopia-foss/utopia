#ifndef UTOPIA_MODELS_HDFBENCH_HH
#define UTOPIA_MODELS_HDFBENCH_HH

#include <map>
#include <vector>
#include <chrono>
#include <limits>
#include <functional>

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
    using BenchmarkFunc = std::function<const double(Config)>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The map of implemented benchmarks and corresponding functions
    std::map<std::string, BenchmarkFunc> _benchmark_funcs;
    
    /// The corresponding setup functions
    std::map<std::string, BenchmarkFunc> _setup_funcs;
    
    /// The configuration for each of the benchmarks and its setup function
    const Config _benchmark_cfgs;
    const Config _setup_cfgs;

    /// The names of enabled benchmarks, read from configuration
    const std::vector<std::string> _enabled;

    /// The results of the measurements, stored under the benchmark name
    std::map<std::string, double> _times;
    
    // -- Datasets -- //
    /// Dataset to store the write times in
    std::shared_ptr<DataSet> _dset_times;

    /// Dataset to write test data to are stored in a map of dataset pointers
    std::map<std::string, std::shared_ptr<DataSet>> _dsets;


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

        // Set maps for benchmark functions and setup functions
        _benchmark_funcs({{"simple_1d", bench_simple_1d}}),
        _setup_funcs(    {{"simple_1d", setup_simple_1d}}),

        // ...and the configurations for each of these
        _benchmark_cfgs(as_<Config>(this->_cfg["benchmark_cfgs"])),
        _setup_cfgs(as_<Config>(this->_cfg["setup_cfgs"])),

        // Get the vector of enabled benchmarks from the config
        _enabled(as_vector<std::string>(this->_cfg["enabled"])),

        // Create the temporary map for measured times and its dataset
        _times(),
        _dset_times(this->_hdfgrp->open_dataset("times"))
    {
        // Check if the benchmark names are valid
        // TODO check for valid names
        // TODO check for duplicates
        for (auto &bname : _enabled) {
            if (false) {
                throw std::invalid_argument("Given benchmark name '"+bname+"' "
                                            "is not valid!");
            }

            // Add entries to the map, setting NaNs for safety
            _times[bname] = std::numeric_limits<double>::quiet_NaN();
        }

        // Set dataset capacities for the times dataset
        this->_log->debug("Setting capacity of 'times' dataset to {} x {} ...",
                          this->get_time_max() + 1, _enabled.size());
        _dset_times->set_capacity({this->get_time_max() + 1, _enabled.size()});

        // Carry out the setup functions
        for (auto& bname : _enabled) {
            _times[bname] = this->measure<true>(bname);
        }

        // Write out the times needed for setup
        this->write_data();

        // Write dimension names and coordinates to dataset attributes
        _dset_times->add_attribute<std::array<std::string, 2>>("dims", {"t", "benchmark"});
        _dset_times->add_attribute("coords_benchmark", _enabled);
    }

    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail The "iteration" in this model is the step that _creates_ the
     *          data that is written in the write_data method, i.e.: the times
     *          needed for each of the enabled benchmarks.
     */
    void perform_step () {
        for (auto& bname : _enabled) {
            _times[bname] = this->measure(bname);
        }
    }


    /// Write data
    void write_data () {   
        _dset_times->write(_times.begin(), _times.end(),
                           [](auto& result) { return result.second; });
    }


protected:

    // Helper functions .......................................................

    template<bool setup=false>
    const double measure(const std::string &bname)
    {
        // Get the configuration and the corresponding function
        Config cfg;
        BenchmarkFunc bfunc;

        if constexpr (setup) {
            cfg = _setup_cfgs[bname];
            bfunc = _setup_funcs.at(bname);
        }
        else {
            cfg = _benchmark_cfgs[bname];
            bfunc = _benchmark_funcs.at(bname);
        }

        // Call the function; its return value is the time it took to execute
        return bfunc(cfg);
    }

    const double time_since(const Time start) {
        dsec seconds = Clock::now() - start;
        return seconds.count();
    }
    
    const double time_between(const Time start, const Time end) {
        dsec seconds = abs(end - start);
        return seconds.count();
    }


    // Setup functions ........................................................

    BenchmarkFunc setup_simple_1d = [this](auto cfg){
        return 1.; // TODO
    };

    // Benchmark functions ....................................................

    BenchmarkFunc bench_simple_1d = [this](auto cfg){
        return 1.; // TODO
    };
};


} // namespace HdfBench
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_HDFBENCH_HH
