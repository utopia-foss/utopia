.. _managers:

Cell and Agent Managers
=======================

Here we address some frequently asked questions regarding the ``CellManager`` and ``AgentManager``.

.. contents::
   :local:
   :depth: 2

----

Entity Traits
-------------
What are these trait objects needed for manager setup?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Utopia::CellTraits`` and ``Utopia::AgentTraits`` are structs that define the properties and behaviour of the cells or agents, respectively.
They are passed to the managers to allow them to set up these entities in the desired way. Only two properties need to always be defined: the state type of the entity, and whether it is to be updated synchronously (all agents of a manager at once), or asynchronously (states change directly, regardless of other agents):

.. code-block :: c++

  /// Traits for cells with synchronous update
  using MySyncCell = Utopia::CellTraits<MyCellState, Update::sync>;

  /// Traits for cells with asynchronous update
  using MyAsyncCell = Utopia::CellTraits<MyCellState, Update::async>;

  /// Traits for cells with determination of update via apply_rule call
  using MyManualCell = Utopia::CellTraits<MyCellState, Update::manual>;


Initialize cells and agents
---------------------------
Which ways are there to initialize cells and agents?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are three different ways to initialize cells and agents in your ``CellManager`` or ``AgentManager``. The examples below are for the ``CellManager`` but apply analogously to the ``AgentManager``.


Constructing an initial state from configuration (recommended)
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

For this (recommended) way, the constructor of the state type accepts a ``Utopia::Config&`` node, from which all the information needed for that state can be extracted:

.. code-block :: c++

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

.. code-block :: c++

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

.. code-block :: yaml

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

The same can be done for the agent manager. The respective configuration
entries are listed below:

.. code-block :: yaml

  # model configuration
  ---
  agent_manager:
    initial_num_agents: 10   # has to be given
    initial_position: random # default mode is ``random``,
                             # currently available modes: ``random``
                             # defines how the initial positions are set

    agent_params:          # passed to cell state Config&-constructor
      a_double: 3.14
      a_string: foo
      a_bool: true

  # Other model configuration parameters ...
.. note ::

  As the ``CellManager`` is not finished with construction at this point, it is
  not possible to use any ``CellManager`` features for construction of the
  cells. The cell state constructor should regard itself only with the
  intrinsic properties of the cell.

.. note ::

  For setting up cell states individually for *each* cell, see the :ref:`question regarding the use of random number generators <random_num_q>`.


Constructing an initial state from the default constructor
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

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

  For setting up cell states individually for *each* cell, see the :ref:`question regarding the use of random number generators <random_num_q>`.


Explicit initial state
""""""""""""""""""""""

In this mode, all cells have an identical initial state, which is passed down from the ``CellManager``. Assuming you are setting up the manager as a member of ``MyFancyModel``, this would look something like this:

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

.. _random_num_q:

Can I use a random number generator when constructing cells or agents?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Yes. The respective managers have access to the shared RNG of the model. If cells or agents provide a constructor that allows passing not only a ``const Config&``, but *also* a random number generator, that constructor has precedence over the one that does not allow passing an RNG:

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

Keep in mind to also change the ``CellTraitsRC`` such that the ``CellStateRC`` creation is done with the config constructor and not the default constructor. For this, set the boolean correctly at the end of the template list to ``true``, as explained above:

.. code-block:: c++

  /// Traits for a default-constructible cell state with synchronous update
  using CellTraitsRC = Utopia::CellTraits<CellStateRC, Update::sync, true>;


.. note::

  In order to have a reproducible state for the RNG, Utopia sets the RNG seed
  globally. That is why the RNG needs to be passed *through* all the way down
  to the cell state constructor.

  You should **not** create a new RNG, neither here nor anywhere else.



.. _cell_manager_faq:

``CellManager`` FAQs
--------------------

Neighborhood calculation
^^^^^^^^^^^^^^^^^^^^^^^^

Where and how are neighborhoods calculated?
"""""""""""""""""""""""""""""""""""""""""""
The neighborhood computation does not take place in the ``CellManager`` itself, but in the underlying ``Grid`` object and based on the cells' IDs.
The ``CellManager`` then retrieves the corresponding shared pointers from the IDs and makes them available via the ``neighbors_of`` method.

Can I change the grid discretization? And when should I?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, the grid discretization can be changed.
Currently available are the ``square`` and ``hexagonal`` grid discretizations.
To change this, select the respective ``structure`` and ``neighborhood/mode`` in the ``cell_manager``'s configuration

.. code-block :: yaml

  # model configuration
  ---
  cell_manager:
    grid:
      structure: square   # can be: square or hexagonal
      resolution: 42      # 42 cells per unit length (of space)

    neighborhood:
      mode: Moore         # can be: empty (0), vonNeumann (4), Moore (8) (with square structure)
                          # can be: empty (0), hexagonal (6) (with hexagonal structure)
                          # the number indicates the number of neighbors per cell

    # Other cell_manager configuration parameters ...

  # Other model configuration parameters ...

.. note::

  The ``resolution`` of the ``hexagonal`` discretization is evaluated per unit area (of space), instead of unit length, as the extent of a hexagon is non-isotrop.
  I.e. with a resolution of 32 in 1x1 space, there will be 30 x 34 = 1020 cells.

The grid discretization, together with the respectively available neighborhoods, should be changed when exploring the influence of geometry and cell-connectivity on cell-cell interactions.
In particular, the number of neighbors per cell can be varied between

* 4 (``square`` grid with ``vonNeumann`` neighborhood)
* 6 (``hexagonal`` grid with ``hexagonal`` neighborhood)
* 8 (``square`` grid with ``Moore`` neighborhood)
* 0 (either grid with ``empty`` neighborhood).

On the other hand, the two discretizations differ in how paths in space are constructed when moving from cell to cell:
In the ``vonNeumann`` neighborhood on a ``square`` lattice the 4 next neighborhoods have unit distance, while the diagonal cells are overly far with distance 2.
In the ``Moore`` neighborhood they are too close with distance 1, where the true distance of the cell centers would be :math:`\sqrt{2}`.
In the ``hexagonal`` discretization all neighbors have the true unit distance, however this is only true for paths that are 60° (instead of 90°) apart.

For more details have a look at the `grid implementation <../../doxygen/html/class_utopia_1_1_grid.html>`_ and `this tutorial <http://www-cs-students.stanford.edu/~amitp/game-programming/grids/>`_.


Are neighborhoods computed on the fly, or can I cache them?
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Yes, the ``CellManager`` allows caching the neighborhood computation's result.
This feature can be controlled via the ``compute_and_store`` argument.

If enabled (which is the default), the neighborhood is computed once for each cell, stored, and retrieved upon calls to ``neighbors_of``.
For more information, see `the doxygen documentation <../../doxygen/html/class_utopia_1_1_cell_manager.html>`_.

Having this feature enabled gives a slight performance gain in most situations.
However, if memory is limited, it might make sense to disable it:

.. code-block:: yaml

    cell_manager:
      neighborhood:
        mode: Moore
        compute_and_store: false

.. note::
    In the ``Grid`` itself, the IDs of the cells in the neighborhood are always computed on the fly.
