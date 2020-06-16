
``SEIRD`` — Model of Susceptible-Exposed-Infected-Recovered-Deceased Contagious Disease Spread
==============================================================================================

This is a simple model combining concepts and ideas from the well known `SEIR (Susceptible-Exposed-Infected-Recovered) <https://en.wikipedia.org/wiki/Compartmental_models_in_epidemiology#The_SEIR_model>`_ and `SIRD (Susceptible-Infected-Recovered-Deceased) <https://en.wikipedia.org/wiki/Compartmental_models_in_epidemiology#The_SIRD_model>`_ models adopting them to a spatial 2D grid model. 

Fundamentals
------------
We model a population of agents on a two-dimensional grid of cells that can move randomly or away of infected agents. Each cell can be in one of the following different states: 

- ``empty``: there is no agent on the cell
- ``susceptible``: the agent on the cell is healthy but susceptible to the disease and can be exposed if in contact with the disease
- ``exposed``: the agent on the cell is exposed to the disease, meaning that it can already infect other agents in contact with the disease, however, there are no symptoms yet, thus, the agent gets noticed as infected only after the incubation period
- ``infected``: the agent on the cell is infected and can infect neighboring agents
- ``recovered``: the agent on the cell is recovered, thus, it is immune to the disease. However, it can lose its immunity after a while and get susceptible again.
- ``deceased``: the agent on the cell is deceased. The cell will be empty in the next time step again.
- ``source``: the cells are infection sources, thus, they can transition neighboring agents from the susceptible to the exposed state.
- ``stones``: the cells are stones, thus, they lie around doing nothing but blocking space on the grid. Stones can be used to model spatial heterogeneities for example through clusters or spatial compartmentalization.


Implementation
--------------
The implementation allows for a range of different storylines by changing the parameters. Keep in mind that individual processes can often be disabled by setting probabilities to zero.

Update Rules
^^^^^^^^^^^^
Each time step the cells update their respective states according to the
following rules:

1. A living (susceptible, exposed, infected, or recovered) cell becomes empty with probability ``p_empty``.
2. An ``empty`` cell turns into a ``susceptible`` one with probability ``p_susceptible``. With probability ``p_immune`` the cell is ``immune`` to being infected.
3. A ``susceptible`` cell either randomly becomes exposed with probability ``p_exposure`` or becomes exposed with probability ``1-p_random_immunity`` if a neighboring cell is ``exposed`` or ``infected``if the cell is _not_ ``immune``.
4. An ``exposed`` cell becomes ``infected`` after an ``incubation_period``.
5. An ``infected`` cell recovers with probability ``p_recover`` and becomes ``immune``, becomes ``deceased`` with probability ``p_decease``, or else stays infected.
6. A ``recovered`` cell can lose its immunity with ``p_lose_immunity`` and becomes ``susceptible`` again 
7. A ``deceased`` cell turns into an ``empty`` cell

Movement
^^^^^^^^
Each time step the agents on the cells can move to ``empty`` neighboring cells according to the following rules:

1. A living (susceptible, exposed, infected, or recovered) cell moves with probability ``p_move_randomly`` to a randomly chosen ``empty`` neighboring cell if there is any.
2. A living cell moves away from an ``infected`` neighboring cell to a randomly selected neighboring ``empty`` cell if there is any.

Heterogeneities
^^^^^^^^^^^^^^^
As in the :doc:`Forest Fire model <ForestFire>`, there is the possibility to introduce heterogeneities into the grid that are implemented as two additional possible cell states:

* ``source``: These are constant exposure sources. They spread the infection like normal infected or exposed cells, but don't revert to the empty state. If activated, they are per default on the lower boundary of the grid. 
* ``stone``: Stones are cells that can't be exposed nor turn into trees. They are used to represent barriers. If enabled, the default mode is ``clustered_simple``, which leads to randomly distributed stones whose neighbors have a certain probability to also be a stone.

Both make use of the :ref:`entity selection interface <entity_selection>`.

Immunity Control
^^^^^^^^^^^^^^^^
Via the ``immunity_control`` parameter in the model configuration, additional immunities can be introduced at desired times manipulating a cells ``immune`` state. This feature can be used to investigate for example the effect of vaccination.

The immunities are introduced before the update rule above is carried out.

Exposure Control
^^^^^^^^^^^^^^^^
Via the ``exposure_control`` parameter in the model configuration, additional exposures can be introduced at desired times.

The exposures are introduced before the update rule above is carried out.

Data Output
^^^^^^^^^^^
The following data is stored alongside the simulation:

* ``kind``: the state of each cell. Possible values:

   * ``0``: ``empty``
   * ``1``: ``susceptible``
   * ``2``: ``exposed``
   * ``3``: ``infected``
   * ``4``: ``recovered``
   * ``5``: ``deceased``
   * ``6``: ``source``, is constantly ignited
   * ``7``: ``stone``, does not take part in any interaction 

* ``age``: the age of each cell, reset after a cell gets empty or deceases
* ``cluster_id``: a number identifying to which cluster a cell belongs; ``0`` for non-living cells. Recovered cells do not count into it.
* ``densities``: the densities of each of the kind of cells over time; this is a labeled 2D array with the dimensions ``time`` and ``kind``.
* ``exposed_time``: the time steps a living cell is already exposed to the disease for each cell
* ``immunity``: whether a cell is immune or not for each cell

Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``SEIRD`` model.

.. literalinclude:: ../../src/utopia/models/SEIRD/SEIRD_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``SEIRD`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/SEIRD/SEIRD_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/SEIRD/SEIRD_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :doc:`here </frontend/inc/base_plots_cfg>`.
