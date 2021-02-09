.. _model_Environment:

``Environment`` — Generic Parameter Background Model
====================================================

The ``Environment`` model is designed to handle changing external parameters
and the initialization and update of non-uniform parameter backgrounds to any
spatial model.
To that end, the model associates its own ``CellManager`` with the
``CellManager`` of the parent model, i.e. it uses the configuration from the
parent's ``CellManager`` to build its own. A cell in the parent model is then
linked to the corresponding cell in the ``Environment`` model, which provides
access to its state. If only the uniform parameter is used, this is optional.

This model provides one library of so-called "environment parameter functions"
that can be used to modify the uniform parameter values in time, and a second
library of so-called "environment state functions" that can be used to set and
modify cell-wise (heterogeneous) parameters. It is also possible to add custom
environment functions of both types.
For the environment state functions, it differentiates between functions which
are invoked once at initialization and others which are invoked during
iteration. Furthermore, it is possible to control the times at which these
functions are invoked.
Additionally, a select config can be added within the state function
configuration, which is passed to the ``select_cells`` feature of the
``CellManager`` (see :ref:`entity_selection`).

The ``Environment`` model is an example of how Utopia models can be nested in
each other.


Adding an ``Environment`` to your own model
-------------------------------------------

The following is a guide to give a finished CA-model access to a global
parameter and a non-uniform parameter background, or, in other words: an
environment.
The model is considered to have a global parameter named
``some_global_parameter`` and a heterogeneous parameter named
``some_heterogeneous_parameter`` which were up to now constant in time and
spatially uniform, i.e. of the same value for all cells.
Of course it is possible to use only either of the two types of parameters and
to have several global or heterogeneous parameters.

First things first: include the header of the Environment model:

.. code-block:: cpp

    #include <utopia/models/Environment/Environment.hh>

Create your Environment's global parameter, adding any parameters you consider part of the environment.

.. code-block:: cpp

    // Use the dummy type EnvParam if you want only to use heterogeneous
    // parameter:
    // using EnvParam = Environment::DummyEnvParam;

    /// Parameter of the Environment model
    /** NOTE this needs to inherit from the Environment::BaseEnvParam
     */
    struct EnvParam : Utopia::Models::Environment::BaseEnvParam
    {
        double some_global_parameter;
        // .. Add more parameters of type double here ..

        EnvParam(const Utopia::DataIO::Config& cfg)
        :
            some_global_parameter(
                Utopia::get_as<double>("some_global_parameter",cfg, 0.))
            // .. initialize additional parameters here ..
        { }

        ~EnvParam() = default;

        /// Getter
        double get_env(const std::string& key) const override {
            if (key == "some_global_parameter") {
                return some_global_parameter;
            }
            // .. Add the new keys here using else if ..
            throw std::invalid_argument("No access method for key '" + key
                                        + "' in EnvParam!");
        }

        /// Setter
        void set_env(const std::string& key,
                    const double& value) override
        {
            if (key == "some_global_parameter") {
                some_global_parameter = value;
            }
            // .. Add the new keys here using else if ..
            else {
                throw std::invalid_argument("No setter method for key '" + key
                                            + "' in EnvParam!");
            }
        }
    };

Create your Environment's cell state, adding any *heterogeneous*
parameters you consider part of the environment.

.. code-block:: cpp

    // Use the dummy type EnvCellState if you want only to use heterogeneous
    // parameter:
    // using EnvCellState = Environment::DummyEnvCellState;

    /// State of the Environment model
    /** NOTE this needs to inherit from the Environment::BaseEnvCellState,
     *       which gives the State access to its position in space.
     */
    struct EnvCellState : Utopia::Models::Environment::BaseEnvCellState {
        /// The some_heterogeneous_parameter
        double some_heterogeneous_parameter;

        // Can add more parameters here ...

        /// Construct the cell state
        /** \details Uses the cell_manager.cell_params entry of your model
         */
        EnvCellState(const Utopia::DataIO::Config& cfg)
        :
            some_heterogeneous_parameter(
                get_as<double>("some_heterogeneous_parameter", cfg, 0.))
            // .. Add more initializations here ..
        { }

        ~EnvCellState() { }

        /// Getter
        double get_env(const std::string& key) const override {
            if (key == "some_heterogeneous_parameter") {
                return some_heterogeneous_parameter;
            }
            // can put more keys here when using more parameters
            // use else if
            else {
                throw std::invalid_argument("No parameter '"+ key +
                                            "' available in EnvCellState!");
            }
        }

        /// Setter
        void set_env(const std::string& key, const double& value) override {
            if (key == "some_heterogeneous_parameter") {
                some_heterogeneous_parameter = value;
            }
            // can put more keys here when using more parameters
            // use else if
            else {
                throw std::invalid_argument("No parameter '"+ key +
                                            "' available in EnvCellState!");
            }
        }
    };

Next, create the link that is used to access the associated cell in the environment:

.. code-block:: cpp

    using EnvModel = Environment::Environment<EnvParam, EnvCellState>;

    // NOTE you do not need the following if you use the DummyEnvCellState

    using EnvCell = EnvModel::CellManager::Cell;

    /// The type of the link container of cells in the Environment model
    template<typename>
    struct EnvLinks {
        /// Link to the associated cell in the Environment model
        std::shared_ptr<EnvCell> env;
    };

If you use the EnvCellState: pass the created link to the ``CellTraits`` in your model, adapting this to the current definitions in your model. The last entry, ``EnvLinks``, is important. If your current model does not have one or more of the entries shown below, use the values given in this example, i.e. the trait's default values.

.. code-block:: cpp

    using CellTraits = Utopia::CellTraits<MyModelCell,   // models cell state
                            Update::sync,  // update mode
                            // whether to use the default constructor
                            false,
                            EmptyTag,      // cell tags
                            EnvLinks>;     // -- the links --

The following changes will need to happen in your model's class, as it will be
the parent to the Environment model.

First, you will need to add a (private) member to your model, preferably somewhere
close to where you currently define the ``CellManager`` of your model:

.. code-block:: cpp

    // -- Members of this model -- //
    /// The cell manager
    CellManager _cm;

    /// The Environment model
    EnvModel _envm;

    /// You might want to keep the global parameter
    double _some_global_parameter;
    // NOTE you need to manually synchronize it with the environment!

    // ...

Then, within the constructor of your model, the constructor of the ``EnvModel``
must be called. Again, this should happen shortly after the construction of the
``CellManager``. The ``EnvModel`` receives your ``CellManager`` as an argument, because it uses its configuration in order to create a corresponding grid representation. It then associates the cells with each other, such that the ``env`` member is set correctly.

.. code-block:: cpp

    /// Construct your model
    template<class ParentModel>
    YourModel (const std::string name, ParentModel& parent)
    :
        Base(name, parent),

        // Initialize the CellManager
        _cm(*this),

        // Initialize the Environment, providing it with that cell manager
        _envm("Environment", *this, _cm),
        // NOTE you can forgo without associated cm if you use the
        //      DummyEnvCellState:
        // _envm("Environment", *this),

        // ...

        {
            // ...

            // Track parameters; these will be saved to the HDF5 file
            _envm.track_parameter("some_global_parameter");
            _envm.track_state("some_heterogeneous_parameter");
            // .. add your additional ones here ..

            // or alternatively: multiple ones
            // _envm.track_parameters({"some_global_parameter",
                                       "some_other_glob_parameter"});
            // _envm.track_states({"some_heterogeneous_parameter",
                                       "some_other_het_parameter"});
        }

.. warning::

  Make sure to iterate the ``Environment`` model when
  performing a step. This should happen first, to ensure that the timings of
  both models correspond.

.. code-block:: cpp

    void perform_step () {
        _envm.iterate();

        // Synchronize your global parameter
        _some_global_parameter = _envm.get_parameter(
                                        "some_global_parameter");

        // ... all the rest of your perform_step method
    }

You can access the parameters in the following way:

.. code-block:: cpp

    RuleFunc update = [this](const auto& cell)
    {
        // The cell's state
        auto state = cell->state;

        // The environment cell's state
        auto env_state = cell->custom_links().env->state;

        // use the synchronized some_global_parameter

With this, your model is ready to use and just needs to be configured!

At some point, coupled models will require manually writing the prolog and epilog:

.. code-block:: cpp

    /// Call the prolog of the sub-models and the model defaults
    void prolog () {
        _envm.prolog(); // prolog of the submodel
        // NOTE the prolog has to be called before starting the iteration of a
        //      submodel. This can be at any time, but usually it will be here.

        this->__prolog();; // default prolog
    }

    /// Call the epilog of the sub-models
    /** NOTE this overwrites the default of your model's epilog. So make
     *  sure that everything that needs to be done has been done.
     */
    void epilog () {
        _envm.epilog(); // epilog of the submodel
        // NOTE the epilog should be called at the end of the submodel's
        //      iteration. This can be at any time, but typically it will be here.

        this->__epilog(); // default epilog
    }


Configuring the Environment model
---------------------------------

To restore the uniform configuration of your model, simply move the
``some_heterogeneous_parameter`` key to the parameters of your cell initialization.
This works even if you don't initialize your model from the configuration, because
the ``Environment`` model inherits the full CellManager configuration.

.. code-block:: yaml

    YourModel:
      some_global_parameter: 3.14

      cell_manager:
        cell_params:
          some_heterogeneous_parameter: 0.2

        ..

Now you can use the Environment model to introduce non-uniformities into your
model, e.g. initialize a spatial gradient within ``some_heterogeneous_parameter``.

.. code-block:: yaml

    YourModel:
      # ...

      # Environment configuration
      Environment:
        # provide some parameter functions
        env_param_funcs:
          - set:
              some_global_parameter:
                value: 20
                times: [0]
                invoke_at_initialization: True
          - sigmoidal:
              some_global_parameter:
                amplitude: 10
                period: 20
                phase: 0.5
                offset: 10
                # NOTE this is not invoked at initialization
        # provide some state functions called at initialization
        init_env_state_funcs:
          # provide a sequence of functions
          - uniform:
              some_heterogeneous_parameter:
                # overwrite the default initialization
                mode: set
                value: 0.0
          - slope:
              some_heterogeneous_parameter:
                mode: add
                values_north_south: [1., 0.]
                # interpolate linearly between these two values

        # provide some state functions for updates
        env_state_funcs:
          - slope:
              some_heterogeneous_parameter:
                mode: set
                values_north_south: [0., 1.]  # flip the slope
                times: [2]                    # ... in the second iteration
          - uniform:
              some_heterogeneous_parameter:
                mode: add
                value: 0.1
                select: # this function is applied only to a selection of cells
                  mode: sample
                  generate: once    # generate a selection that is fixed
                  # can also be 'always' to generate every invocation
                  num_cells: 4
                  fix_selection: false  # generated new for every call


.. warning::

   When using recursive update of the configuration files, make sure to overwrite entries you no longer wish to use. For instance, if you set a default ``env_state_funcs`` in your default model configuration, but do not wish to update the intial environmental state from a run config, write:

    .. code-block:: yaml

        Environment:
          init_env_state_funcs:
            # your init sequence here ...

          env_state_funcs: ~  # overwrites the sequence, and nothing is invoked

.. warning::

    The cell manager is currently set up according to the configuration of the parent's
    cell manager. Unfortunately, this does not include information about the
    space, hence make sure to also copy the space config to the
    Environment's configuration, e.g. using yaml anchors, or the association of
    the two cell managers will fail!

.. hint::

    If you are unsure which environment functions are set up and are invoked at
    which point in time, call ``utopia run`` with the ``--debug`` flag, which
    will increase log verbosity and tell you when environment functions are
    called. Alternatively, you can add the ``log_level: debug`` or ``log_level: trace``
    entry to the ``Environment`` model entry in the configuration.
    
.. note::

    You can still use the features of the model even if your current model does not use the ``cell_manager``.
    If you only use the ``EnvParam``, just use the model without an associated cell manager (see the above description of how to change to constructor of your model).
    If you want to use the ``EnvCellState``, pass the template argument ``associate=false``; the model can then be initialized without an associated cell manager.
    However, in all of these cases **you must configure the** ``cell_manager`` within the ``Environment`` node of your configuration:

    .. code-block:: yaml

        Environment:
          cell_manager:
            grid:
              structure: square
              resolution: 1
            neighborhood:
              mode: empty
            cell_params: ~


          # Continue as described above

Collection of parameter and state functions
-------------------------------------------

Currently implemented environment functions are available in the two
collections ``ParameterFunctionCollection`` and ``StateFunctionCollection``.
Please check their documentations for the configuration of the respective
function.


Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``Environment`` model.
These include an example of how environment functions can be set up.

.. literalinclude:: ../../src/utopia/models/Environment/Environment_cfg.yml
   :language: yaml
   :start-after: ---
