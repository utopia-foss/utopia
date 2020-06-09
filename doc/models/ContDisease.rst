
``ContDisease`` — Model of Contagious Diseases
==============================================

This is a simple model of a contagious disease on a 2D grid. It is based
on the description in the script of the CCEES lecture of Prof. Roth.

Fundamentals
------------
We model a "forest" on a two-dimensional square grid of cells. Each cell can be
in one of five different states: empty, tree, infected, source or stones.

Implementation
--------------
Update Rules
^^^^^^^^^^^^
Each time step the cells update their respective states according to the
following rules:

1. An ``infected`` cell turns into an ``empty`` cell
2. An ``empty`` cell can become a ``tree`` cell with probability ``p_growth``.
3. A ``tree`` cell can become infected in the following ways:

   - From a neighboring infected cell with probability 1-``p_immunity``
     *per neighbor*
   - Via a random point infection with probability ``p_infect``
   - Via a constantly infected cell, an infection ``source``

For the neighborhood, both the von Neumann neighborhood (5-neighborhood) and the Moore neighborhood (9-neighborhood) are supported (see model configuration).

Heterogeneities
^^^^^^^^^^^^^^^
As in the :doc:`Forest Fire model <ForestFire>`, there is the possibility to introduce heterogeneities into the grid, which are implemented as two additional possible cell states:

* ``source``: These are constant infection sources. They spread infection like normal infected trees, but don't revert back to the empty state. If activated, they are per default on the lower boundary of the grid. 
* ``stone``: Stones are cells that can't be infected nor turn into trees. They are used to represent barriers in the forest. If enabled, the default mode is ``clustered_simple``, which leads to randomly distributed stones whose neighbours have a certain probability to also be a stone.

Both make use of the :ref:`entity selection interface <entity_selection>`.

Infection Control
^^^^^^^^^^^^^^^^^
Via the ``infection_control`` parameter in the model configuration, additional infections can be introduced at desired times.

The infections are introduced before the update rule above is carried out.

Data Output
^^^^^^^^^^^
The following data is stored alongside the simulation:

* ``kind``: the state of each cell. Possible values:

   * ``0``: ``empty``
   * ``1``: ``tree``
   * ``2``: ``infected``
   * ``3``: ``source``, is constantly ignited
   * ``4``: ``stone``, does not take part in any interaction 

* ``age``: the age of each tree, reset after lightning strikes
* ``cluster_id``: a number identifying to which cluster a cell belongs; ``0`` for non-tree cells
* ``densities``: the densities of each of the kind of cells over time; this is a labeled 2D array with the dimensions ``time`` and ``kind``.


Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``ContDisease`` model.

.. literalinclude:: ../../src/utopia/models/ContDisease/ContDisease_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``ContDisease`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/ContDisease/ContDisease_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/ContDisease/ContDisease_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :doc:`here </frontend/inc/base_plots_cfg>`.


References
----------

Kurt Roth: Chaotic, Complex, and Evolving Environmental Systems. Unpublished lecture notes, University of Heidelberg, 2018.
