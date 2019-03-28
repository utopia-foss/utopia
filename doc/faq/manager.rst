``CellManager`` and ``AgentManager`` FAQs
=========================================

Below, some frequently asked questions regarding the ``CellManager`` and ``AgentManager`` are addressed.

.. contents::
   :local:
   :depth: 1

----

What are these trait objects needed for manager setup?
------------------------------------------------------

The ``Utopia::CellTraits`` and ``Utopia::AgentTraits`` are structs that define the properties and behaviour of the cells or agents, respectively.
They are passed to the managers to allow them to set up these entities in the desired way.

Only two properties need always be defined: The state type of the entity and whether it is to be updated synchronously (all agents of a manager at once), or asynchronously (states change directly, regardless of other agents):

.. code-block:: c++

  /// Traits for cells with synchronous update
  using MySyncCell = Utopia::CellTraits<MyCellState, Update::sync>;
  
  /// Traits for cells with asynchronous update
  using MyAsyncCell = Utopia::CellTraits<MyCellState, Update::async>;



Which ways are there to initialize cells and agents?
----------------------------------------------------

There are three different ways to initialize cells and agents in your ``CellManager`` or ``AgentManager``. The examples below are for the ``CellManager`` but apply analogously to the ``AgentManager``.


Constructing an initial state from configuration (recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For this (recommended) way, the constructor of the state type accepts a ``Utopia::Config&`` node, where all the information needed for that state can be extracted:

.. code-block:: c++

  /// The cell state type
  struct CellState {
      double a_double;
      std::string a_string;
      bool a_bool;

      /// Config-constructor for the cell state
      CellStateCC(const Config& cfg)
      :
          a_double(Utopia::get_as<double>("a_double", cfg)),
          a_string(Utopia::get_as<std::string>("a_string", cfg)),
          a_bool(Utopia::get_as<bool>("a_bool", bool))
      {}
  };

  /// Traits for a config-constructible cell state with synchronous update
  using CellTraitsCC = Utopia::CellTraits<CellStateCC, Update::sync>;

The manager itself can then be set up in the model without any further information:

.. code-block:: c++

  class MyFancyModel {
  public:
      // MyFancyModel member definitions
      CellManager<CellTraitsCC, MyFancyModel> _cm;

      /// MyFancyModel constructor
      template<class ParentModel>
      MyFancyModel (const std::string name, ParentModel &parent)
      :
          // ...
          // Set up the cell manager _just_ using the model
          _cm(*this),
          // ...
      {}
  };

Here, the cell manager extracts the required information from the model configuration.
It expects a configuration entry ``cell_manager``, which includes all the information needed for setup, including those parameters passed to the ``Config&`` constructor:

.. code-block:: yaml

  # model configuration
  ---
  cell_manager:
    grid:                 # grid properties
      structure: square   # cells should be square
      resolution: 42      # 42 cells per unit length (of space)

    neighborhood:
      mode: Moore         # can be: empty, vonNeumann, Moore

    cell_params:          # passed to cell state Config&-constructor
      a_double: 3.14
      a_string: foo
      a_bool: true

  # Other model configuration parameters ...


.. note::

  As the ``CellManager`` is not finished with construction at this point, it is
  not possible to use any ``CellManager`` features for construction of the
  cells. The cell state constructor should regard itself only with the
  intrinsic properties of the cell.

.. note ::

  For setting up cell states individually for *each* cell, see the question regarding use of random number generators.


Constructing initial state from default constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As default constructors can sometimes lead to undefined behaviour, they need to be explicitly allowed. This happens via the ``Utopia::CellTraits`` struct.

.. code-block:: c++

  /// A cell state definition that is default-constructible
  struct CellStateDC {
      double a_double;
      std::string a_string;
      bool a_bool;

      CellStateDC()
      :
          a_double(3.14), a_string("foo"), a_bool(false)
      {}
  };

  /// Traits for a default-constructible cell state with synchronous update
  using CellTraitsDC = Utopia::CellTraits<CellStateDC, Update::sync, true>;

In such a case, the manager (as with config-constructible) does not require an initial state.

.. note ::

  For setting up cell states individually for *each* cell, see the question regarding use of random number generators.


Explicit initial state
^^^^^^^^^^^^^^^^^^^^^^

In this mode, all cells have an identical initial state, which is passed down from the ``CellManager``. Presuming you are setting up the manager as member of ``MyFancyModel``, this would look something like this:

.. code-block:: c++

  /// The cell state type
  struct MyCellState {
      int foo;
      double bar;
  }

  /// Traits for cells with synchronous update
  using MyCellTraits = Utopia::CellTraits<MyCellState, Update::sync>;

  // Define an appropriate initial cell state
  const auto initial_cell_state = MyCellState(42, 3.14);

  // ...

  class MyFancyModel {
  public:
      // MyFancyModel member definitions
      CellManager<MyCellTraits, MyFancyModel> _cm;

      /// MyFancyModel constructor
      template<class ParentModel>
      MyFancyModel (const std::string name, ParentModel &parent)
      :
          // ...
          // Set up the cell manager, passing the initial cell state
          _cm(*this, initial_cell_state),
          // ...
      {}
  };


Can I have a random number generator when constructing cells or agents?
-----------------------------------------------------------------------

Yes.

The respective managers have access to the shared RNG of the model.
If cells or agents provide a constructor that allows passing not only a ``const Config&``, but *also* a random number generator, that constructor has precedence over the one that does not allow passing an RNG:

.. code-block:: c++

  /// A cell state definition that is config-constructible and has an RNG
  struct CellStateRC {
      double a_double;
      std::string a_string;
      bool a_bool;
  
      // Construct a cell state with the use of a RNG
      template<class RNGType>
      CellStateRC(const Config& cfg, const std::shared_ptr<RNGType>& rng)
      :
          a_double(Utopia::get_as<double>("a_double", cfg)),
          a_string(Utopia::get_as<std::string>("a_string", cfg))
      {
          // Do something with the RNG to set the boolean
          std::uniform_real_distribution<double> dist(0., 1.);
          a_bool = (dist(*rng) < a_double);
      }
  };

With this constructor available, a constructor with the signature ``CellStateRC(const Config& cfg)`` is not necessary and would *not* be called!

.. note::

  In order to have a reproducible state for the RNG, Utopia sets the RNG seed
  globally. That is why the RNG needs to be passed *through* all the way down
  to the cell state constructor.

  You should **not** create a new RNG; not here, not anywhere.
