#ifndef UTOPIA_TEST_MODEL_NESTED_TEST_HH
#define UTOPIA_TEST_MODEL_NESTED_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

namespace Utopia {

/// Define data types for use in all models
using CommonModelTypes = ModelTypes<
    std::vector<double>,
    std::vector<double>
>;


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

    /// store the level as a member
    const unsigned int level;

public:
    /// Constructor
    template<class ParentModel>
    DoNothingModel (const std::string name,
                    ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Store level
        level(cfg["level"].as<unsigned int>())
    {
        std::cout << "  DoNothingModel '" << name << "' initialized. "
                  << "Level: " << level << std::endl;
    }

    /// Perform a single step (nothing to do here)
    void perform_step () {}

    /// Data write method (does nothing here)
    void write_data () {}

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

    /// store the level as a member
    const unsigned int level;

    /// submodel: DoNothingModel
    DoNothingModel lazy;

public:
    /// Constructor
    template<class ParentModel>
    OneModel (const std::string name,
              ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Store level
        level(cfg["level"].as<unsigned int>()),
        // Submodel initialization
        lazy("lazy", *this)
    {
        std::cout << "  OneModel '" << name << "' initialized. "
                  << "Level: " << level << std::endl;
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        lazy.iterate();
    }

    /// Data write method (does nothing here)
    void write_data () {}

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

    /// store the level as a member
    const unsigned int level;

    /// submodel: One
    OneModel sub_one;

    /// submodel: DoNothing
    DoNothingModel sub_lazy;

public:
    /// Constructor
    template<class ParentModel>
    AnotherModel (const std::string name,
                  ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        // Store level
        level(cfg["level"].as<unsigned int>()),
        // Submodel initialization
        sub_one("one", parent_model),
        sub_lazy("lazy", *this)
    {
        std::cout << "  AnotherModel '" << name << "' initialized. "
                  << "Level: " << level << std::endl;
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        sub_one.iterate();
        sub_lazy.iterate();
    }

    /// Data write method (does nothing here)
    void write_data () {}

};


/// The RootModel is a model that implement other models within it
class RootModel:
    public Model<RootModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<RootModel, CommonModelTypes>;

    /// store the level as a member
    const unsigned int level;

    /// submodel: OneModel
    OneModel sub_one;

    /// submodel: AnotherModel
    AnotherModel sub_another;

public:
    /// Create RootModel with initial state
    template<class ParentModel>
    RootModel (const std::string name,
               ParentModel &parent_model)
    :
        // Initialize completely via parent class constructor
        Base(name, parent_model),
        // Store level
        level(cfg["level"].as<unsigned int>()),
        // Submodel initialization
        sub_one("one", *this),
        sub_another("another", *this)
    {
        std::cout << "  RootModel '" << name << "' initialized. "
                  << "Level: " << level << std::endl;
    }

    /// Perform a single step, i.e.: iterate the submodels
    void perform_step ()
    {
        sub_one.iterate();
        sub_another.iterate();
    }

    /// Data write method (does nothing here)
    void write_data () {}

};


} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_NESTED_TEST_HH
