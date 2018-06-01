#ifndef UTOPIA_TEST_MODEL_SETUP_TEST_HH
#define UTOPIA_TEST_MODEL_SETUP_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>

namespace Utopia {

/// Define a dummy data type
using DoNothingModelType = ModelTypes<bool, bool>;

/// A model that does nothing
class DoNothingModel:
    public Model<DoNothingModel, DoNothingModelType>
{
public:
    /// The base model class
    using Base = Model<DoNothingModel, DoNothingModelType>;

    /// Granular constructor
    DoNothingModel (const std::string name,
                    Config &parent_cfg,
                    std::shared_ptr<DataGroup> parent_group,
                    std::shared_ptr<RNG> shared_rng)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_cfg, parent_group, shared_rng)
    {
        std::cout << "DoNothingModel '" << name
                  << "' initialized. " << std::endl;
    }


    /// Constructor via parent model
    template<class ParentModel>
    DoNothingModel (const std::string name,
                    ParentModel &parent_model)
    :
        // Pass arguments to the corresponding base class constructor
        Base(name, parent_model)
    {
        std::cout << "DoNothingModel '" << name
                  << "' initialized via parent model. " << std::endl;
    }


    /// Perform a single step (nothing to do here)
    void perform_step () {}

    /// Data write method (does nothing)
    void write_data () {}
};

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_SETUP_TEST_HH
