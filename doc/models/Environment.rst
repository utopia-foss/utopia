
``Environment`` — Model for non-uniform parameter background
============================================================

The ``Environment`` model was designed to handle the initialization and update
of non-uniform parameter backgrounds to any spatial model. 
To that end, the model associates its own ``CellManager`` with the
``CellManager`` of the parent model, i.e. it uses the configuration from the
parent's ``CellManager`` to build its own. A cell in the parent model is then
linked to the corresponding cell in the ``Environment`` model, which provides
access to its state.

This model provides a library of so-called "environment functions" that can be
used to set and modify the parameter values. It is also possible to add custom
environment functions.
It differentiates between environment functions which are invoked once at
initialization and others which are invoked during iteration. Furthermore, it
is possible to control the times at which these functions are invoked.

The ``Environment`` model is an example of how Utopia models can be nested in
each other.


Adding an ``Environment`` to your own model
-------------------------------------------

The following is a guide to give a finished CA-model access to a non-uniform
parameter background, or, in other words: an environment.
The model is considered to have a parameter named ``some_parameter`` that was
up to now spatially uniform, i.e. of the same value for all cells.

First things first: include the header of the Environment model:

.. code-block:: cpp

    #include "../Environment/Environment.hh"

Create your Environment's cell state. Here, you add those parameters that you
consider part of the environment.

.. code-block:: cpp

    /// State of the Environment model
    /** NOTE this needs to inherit from the Environment::BaseEnvCellState,
     *       which gives the State access to its position in space.
     */
    struct EnvCellState : Utopia::Models::Environment::BaseEnvCellState {
        /// The some_parameter
        double some_parameter;

        // Can add more parameters here ...

        /// Construct the cell state
        /** \detail Uses the cell_manager.cell_params entry of your model
         */
        EnvCellState(const DataIO::Config& cfg)
        :
            some_parameter(get_as<double>("some_parameter", cfg, 0.))
        { }

        ~EnvCellState() { }

        /// Getter
        double get_env(const std::string& key) {
            if (key == "some_parameter") {
                return some_parameter;
            }
            // can put more keys here when using more parameters
            // use else if
            else {
                throw std::invalid_argument("No parameter '"+ key +
                                            "' available in EnvCellState!");
            }
        }

        /// Setter
        void set_env(const std::string& key, const double& value) {
            if (key == "some_parameter") {
                some_parameter = value;
            }
            // can put more keys here when using more parameters
            // use else if
            else {
                throw std::invalid_argument("No parameter '"+ key +
                                            "' available in EnvCellState!");
            }
        }
    };

Create the link that is used to access the associated cell in the environment:

.. code-block:: cpp

    using Utopia::Models::Environment::Environment;
    using EnvModel = Environment<EnvCellState>;
    using EnvCell = EnvModel::CellManager::Cell;

    /// The type of the link container of cells in the Environment model
    template<typename>
    struct EnvLinks {
        /// Link to the associated cell in the Environment model
        std::shared_ptr<EnvCell> env;
    };

Pass the created link to the ``CellTraits`` in your model; adapt this to the
current definitions in your model.
Important is the last entry, ``EnvLinks``. If your current model does not have
one or more of the entries shown below, use the values given in this example,
i.e. the trait's default values.

.. code-block:: cpp

    using CellTraits = Utopia::CellTraits<MyModelCell,   // models cell state
                                          Update::sync,  // update mode
                                          false,         // whether to use the default constructor
                                          EmptyTag,      // cell tags
                                          EnvLinks>;     // -- the links --

The following changes will need to happen in your models class as it will be
the parent to the Environment model.

First, you will need to add a (private) member to your model, best somewhere
close to where you currently define the ``CellManager`` of your model:

.. code-block:: cpp

    // -- Members of this model -- //
    /// The cell manager
    CellManager _cm;

    /// The Environment model
    EnvModel _envm;

    // ...

Then, within the constructor of your model the constructor of the ``EnvModel``
must be called. Again this should happen close to the construction of the
``CellManager``, but afterwards.
The ``EnvModel`` gets your ``CellManager`` as an argument, because it uses
its configuration in order to create a corresponding grid representation. It
then associates the cells with each other, such that the ``env`` member is
set correctly.

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

        // ...

        {
            // ...

            // Track parameters; these will be saved to the HDF5 file
            _envm.track_parameter("some_parameter");

            // or alternatively: multiple ones
            // _envm.track_parameters({"some_parameter", "some_other"});

            // Write the initial data of the env model
            if (_envm.get_write_start() == 0) {
                _envm.write_data();
                // NOTE Need this because env model is never run, but only 
                //      iterated
            }
        }

.. note::

  **Important:** Make sure you iterate the ``Environment`` model when
  performing a step. This should happen first to ensure, that the timings of
  both models corresponds.

.. code-block:: cpp

    void perform_step () {
        _envm.iterate();

        // ... all the rest of your perform_step method
    }

With this, your model is ready to use and just needs to be configured.


Configuring the Environment model
---------------------------------

To restore the uniform configuration of your model just move the
``some_parameter`` key to the parameters of your cell initialization.
This works even if you don't initialize your model from configuration, because
the ``Environment`` model inherits the full CellManager configuration.

.. code-block:: yaml

    YourModel: !model
      model_name: YourModel

      cell_manager:
        cell_params:
          some_parameter: 0.2

        ..

Now you can use the Environment model to introduce non-uniformities to your
model, e.g. initialize a spatial gradient within ``some_parameter``.

.. code-block:: yaml

    YourModel: !model
      # ...

      # Environment configuration
      Environment:
        # provide some functions called at initialization
        init_env_funcs:
          # provide a sequence of functions
          - uniform:
              some_parameter:
                # overwrite the default initialization
                mode: set
                value: 0.0
          - slope:
              some_parameter:
                mode: add
                values_north_south: [1., 0.] 
                # interpolate linearly between these two values

        # provide some functions for updates
        env_funs:
          - slope:
              some_parameter:
                mode: set
                values_north_south: [0., 1.]  # flip the slope
                times: [2]                    # ... in the second iteration

.. note::

    Make sure that when using recursive update of the configuration files to
    overwrite entries that you no longer wish to use. 
    So, if you define default ``env_funcs`` in your default model config, but
    don't want to update the intial environmental state from a run config
    write:

    .. code-block:: yaml

        Environment:
          init_env_funcs:
            # your init sequence here ...

          env_funcs: ~  # overwrites the sequence -> nothing invoked

.. note::

    If you are unsure which environment functions are set up and are invoked at
    which point in time, call ``utopia run`` with the ``--debug`` flag, which
    will increase log verbosity and tell you when environment functions are
    called.
    Alternatively, you can add the ``log_level: debug`` or ``log_level: trace``
    entry to the ``Environment`` model entry in the configuration.


Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``Environment`` model.
These include an example of how environment functions can be set up.

.. literalinclude:: ../../src/models/Environment/Environment_cfg.yml
   :language: yaml
   :start-after: ---
