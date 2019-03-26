#ifndef UTOPIA_TEST_MODEL_SETUP_TEST_HH
#define UTOPIA_TEST_MODEL_SETUP_TEST_HH

#include <utopia/core/model.hh>

namespace Utopia {

/// Define a dummy data type
using DoNothingModelType = ModelTypes<>;

/// A model that does nothing
class DoNothingModel:
    public Model<DoNothingModel, DoNothingModelType>
{
public:
    /// The base model class
    using Base = Model<DoNothingModel, DoNothingModelType>;

    /// Constructor via parent model
    template<class ParentModel>
    DoNothingModel (const std::string name,
                    const ParentModel &parent_model)
    :
        // Pass arguments to the corresponding base class constructor
        Base(name, parent_model)
    {
        std::cout << "DoNothingModel '" << name
                  << "' initialized via parent model. " << std::endl;
    }


    /// Perform a single step (nothing to do here)
    void perform_step () {}

    /// Monitor data (does nothing)
    void monitor () {}

    /// Data write method (does nothing)
    void write_data () {}
};

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_SETUP_TEST_HH
