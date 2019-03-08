
``ContDisease`` - A Model of a Contagious Disease
=================================================

This is a simple model of a contagious disease on a 2D grid. It is based
on the description in the script of the CCEES lecture of Prof. Roth.

Fundamentals
------------

We model a "forest" on a two dimensional square grid of cells. Each cell can be
in one of five different states: empty, tree, infected, herd or stones.

Update Rules
------------

Each time step the cells update their respective states according to the
following rules:

1. An ``infected`` cell turns into an ``empty`` cell
2. An ``empty`` cell can become a ``tree`` cell with probability ``p_growth``.
3. A ``tree`` cell can become infected in the following ways:

   - From a neighbouring infected cell with probability ``p_infect``
     *per neighbor*
   - Via a random point infection with probability ``p_rd_infect``
   - Via a constantly infected cell, an infection ``source``

For the neighborhood, both the von Neumann neighborhood (5-neighborhood) and the Moore neighborhood (9-neighborhood) are supported (see model configuration).

Infection sources
^^^^^^^^^^^^^^^^^

Constant infection sources can be activated at the lower boundary of the grid.
They spread infection like normal infected trees, but don't revert back to the
empty state.

Stones
^^^^^^

Stone are cells that can't be infected nor turn into trees. They are used to
represent barriers in the forest. At the moment they can initialized at random,
with an additional weight to encourage clustering of stones.


Default configuration parameters
--------------------------------

Below are the default configuration parameters for the ``ContDisease`` model.

.. literalinclude:: ../../src/models/ContDisease/ContDisease_cfg.yml
   :language: yaml
   :start-after: ---


Simulation Results
------------------

*TODO*

Theoretical Background
----------------------

*TODO*

Possible Future Extensions
--------------------------
* More ways to initialize stones.


References
----------

Kurt Roth: Chaotic, Complex, and Evolving Environmental Systems. Unpublished lecture notes, University of Heidelberg, 2018.
