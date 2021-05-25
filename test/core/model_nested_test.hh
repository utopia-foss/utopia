#ifndef UTOPIA_TEST_MODEL_NESTED_TEST_HH
#define UTOPIA_TEST_MODEL_NESTED_TEST_HH

#include <utopia/core/model.hh>

namespace Utopia {

/// Define data types for use in all models
using CommonModelTypes = ModelTypes<>;


/// Test model that is used within the nested models
/** This model is used to nest it multiple times within the RootModel class
 *  that is defined below
 */
class DoNothingModel:
    public Model<DoNothingModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<DoNothingModel, CommonModelTypes>;

    /// Whether the prolog was performed
    bool _prolog_run;

    /// Whether the epilog was performed
    bool _epilog_run;

public:
    /// Constructor
    template<class ParentModel>
    DoNothingModel (const std::string name,
                    const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        _prolog_run(false),
        _epilog_run(false)
    {
        this->_log->info("DoNothingModel initialized. Level: {}", this->_level);
    }

    /// Perform a single step (nothing to do here)
    void perform_step () {}

    /// Monitor data (does nothing)
    void monitor () {}

    /// Data write method (does nothing here)
    void write_data () {}

    /// The prolog
    void prolog () {
        if (_prolog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        this->__prolog();

        _prolog_run = true;
    }

    /// The epilog
    void epilog () {
        if (_epilog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        this->__epilog();

        _epilog_run = true;
    }

};


/// Test model that is used within the nested models
/** This model is used to nest it multiple times within the RootModel class
 *  that is defined below
 */
class OneModel:
    public Model<OneModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<OneModel, CommonModelTypes>;

    /// submodel: DoNothingModel
    DoNothingModel lazy;

    /// Whether the prolog was performed
    bool _prolog_run;

    /// Whether the epilog was performed
    bool _epilog_run;

public:
    /// Constructor
    template<class ParentModel>
    OneModel (const std::string name,
              const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Submodel initialization
        lazy("lazy", *this),
        _prolog_run(false),
        _epilog_run(false)
    {
        this->_log->info("OneModel initialized. Level: {}", this->_level);
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        lazy.iterate();
    }

    /// Monitor data (do nothing here)
    void monitor () {}

    /// Data write method (does nothing here)
    void write_data () {}

    /// Prolog
    void prolog () {
        if (_prolog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        // call the submodels prolog
        lazy.prolog();

        // call the own default prolog
        this->__prolog();

        _prolog_run = true;
    }

    /// Epilog (call the epilog of DoNothing)
    void epilog () {
        if (_epilog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        // call the submodel's epilog
        lazy.epilog();

        // call the own default epilog
        this->__epilog();

        _epilog_run = true;
    }

};


/// Another test model that is used within the nested models
/** This model is used to nest it multiple times within the RootModel class
 *  that is defined below
 */
class AnotherModel:
    public Model<AnotherModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<AnotherModel, CommonModelTypes>;

    /// submodel: One
    OneModel another_one;

    /// submodel: DoNothing
    DoNothingModel another_lazy;

    /// Whether the prolog was performed
    bool _prolog_run;

    /// Whether the epilog was performed
    bool _epilog_run;

public:
    /// Constructor
    template<class ParentModel>
    AnotherModel (const std::string name,
                  const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Submodel initialization
        another_one("one", *this),
        another_lazy("lazy", *this),
        _prolog_run(false),
        _epilog_run(false)
    {
        this->_log->info("AnotherModel initialized. Level: {}", this->_level);
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        another_one.iterate();
    }

    /// Monitor data (do nothing here)
    void monitor () {}

    /// Data write method (does nothing here)
    void write_data () {}

    /// Prolog
    void prolog () {
        if (_prolog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        // call the submodels prolog
        another_one.prolog();

        // run the sub_lazy model in its entire length
        another_lazy.run();

        // call the own default prolog
        this->__prolog();

        _prolog_run = true;
    }

    /// Epilog (call the epilog on all submodels)
    void epilog () {
        if (_epilog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        // call the submodels epilog
        another_one.epilog();

        // call the own default epilog
        this->__epilog();

        _epilog_run = true;
    }

};


/// The RootModel is a model that implement other models within it
class RootModel:
    public Model<RootModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<RootModel, CommonModelTypes>;

    /// submodel: OneModel
    OneModel sub_one;

    /// submodel: AnotherModel
    AnotherModel sub_another;

    /// submodel: DoNothingModel
    DoNothingModel sub_idle;

    /// Whether the prolog was performed
    bool _prolog_run;

    /// Whether the epilog was performed
    bool _epilog_run;

    /// Iterate model one to this time
    Time _stop_iterate_one;

    /// Start iterating model another at this time
    Time _start_iterate_another;

public:
    /// Create RootModel with initial state
    template<class ParentModel>
    RootModel (const std::string name,
               const ParentModel &parent_model)
    :
        // Initialize completely via parent class constructor
        Base(name, parent_model),
        // Submodel initialization
        sub_one("one", *this),
        sub_another("another", *this),
        sub_idle("idle", *this),
        _prolog_run(false),
        _epilog_run(false),
        _stop_iterate_one(get_as<Time>("stop_iterate_one", this->_cfg)),
        _start_iterate_another(get_as<Time>("start_iterate_another", this->_cfg))

    {
        this->_log->info("RootModel initialized. Level: {}", this->_level);
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        if (this->_time < _stop_iterate_one - 1) {
            sub_one.iterate();
        }
        else if (this->_time == _stop_iterate_one - 1) {
            sub_one.iterate();
            sub_one.epilog();
        }


        if (this->_time + 1 == _start_iterate_another) {
            if (sub_another._prolog_run) {
                throw std::runtime_error("Prolog of sub_another has been run "
                    "before its due time!");
            }
            sub_another.prolog();
            sub_another.iterate();
        }
        else if (this->_time + 1 > _start_iterate_another) {
            sub_another.iterate();
        }
    }

    /// Monitor data (do nothing here)
    void monitor () {}

    /// Data write method (does nothing here)
    void write_data () {}

    /// Prolog
    void prolog () {
        if (_prolog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        // call the submodels prolog
        sub_one.prolog();

        // call the own default prolog
        this->__prolog();

        _prolog_run = true;
    }

    /// Epilog (call the epilog on all submodels)
    void epilog () {
        if (_epilog_run) {
            throw std::runtime_error(fmt::format("Requesting to run prolog "
               "another time in {} model!", this->get_full_name()));
        }

        if (not sub_one._epilog_run) {
            throw std::runtime_error("Epilog of sub_one has not been run "
                "at its due time!");
        }

        sub_another.epilog();

        // call the own default epilog
        this->__epilog();

        _epilog_run = true;
    }

};


} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_NESTED_TEST_HH
