#ifndef UTOPIA_TEST_MODEL_NESTED_TEST_HH
#define UTOPIA_TEST_MODEL_NESTED_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

namespace Utopia {

/// Define data types for all three models
using CommonModelTypes = ModelTypes<
    std::vector<double>,
    std::vector<double>
>;


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

private:
    // no members needed here

public:
    /// Constructor
    OneModel (const std::string name,
               Config &parent_cfg,
               std::shared_ptr<DataGroup> parent_group,
               std::shared_ptr<RNG> shared_rng)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_cfg, parent_group, shared_rng)
    { }
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

private:
    // no members needed here

public:
    /// Constructor
    AnotherModel (const std::string name,
               Config &parent_cfg,
               std::shared_ptr<DataGroup> parent_group,
               std::shared_ptr<RNG> shared_rng)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_cfg, parent_group, shared_rng)
    { }
};


/// The RootModel is a model that implement other models within it
class RootModel:
    public Model<RootModel, CommonModelTypes>
{
public:
    /// The base model class
    using Base = Model<RootModel, CommonModelTypes>;

private:
    // no members needed here

public:
    /// Create RootModel with initial state
    RootModel (const std::string name,
               Config &parent_cfg,
               std::shared_ptr<DataGroup> parent_group,
               std::shared_ptr<RNG> shared_rng)
    :
        // Initialize completely via parent class constructor
        Base(name, parent_cfg, parent_group, shared_rng)
    { }
};


} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_NESTED_TEST_HH
