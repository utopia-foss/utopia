
``ForestFire`` Model
====================

The ``ForestFire`` model simulates the development of a forest under influence of forest fires. Trees grow on a random basis and fires cause their death, for a whole cluster instantaneously; this is the so-called "two state model".

Fundamentals
------------

The model is based on the description in the CCEES Lecture Notes by Kurt Roth in chapter 7.3 (Discrete Complex Systems - Contact Processes).

It is modelled as a cellular automaton, where each cell can have one of the two states ``empty`` or ``tree``.

Trees grow on an empty cell with probability ``growth_rate``. With the probability ``lightning_frequency`` a cell is ignited and this cell, as well as all cells indirectly connected to it (the cluster) burn instantaneously and turn to empty; a percolation occurs.
The cluster is determined by recursively percolating through the Moore neighbours (by default) of each cell.

Percolation mode
^^^^^^^^^^^^^^^^

There is the possibility to use the model feature ``ignite_bottom_row`` instead of a lightning frequency. In that mode, cells at the bottom (southern) boundary of the grid are constantly ignited.
It makes sense to set the ``lightning_frequency`` to zero in this mode.

.. note::

  In this mode, it is required to make the space non-periodic.


Default configuration parameters
--------------------------------

Below are the default configuration parameters for the ``ForestFire`` model.

.. literalinclude:: ../../src/models/ForestFire/ForestFire_cfg.yml
   :language: yaml
   :start-after: ---


Simulation Results
------------------

*Work in progress.* For now, see CCEES lecture notes.


References
----------

* 
  Bak, P., K. Chen and C. Tang, 1990: A forest-fire model and some thoughts on turbulence, Phys. Lett. A, 147, (5-6), 297–300,
  doi: `10.1016/0375–9601(90)90451–S <https://doi.org/10.1016/0375-9601(90)90451-S>`_ (`PDF <http://cqb.pku.edu.cn/tanglab/pdf/1990-55.pdf>`_).

* 
  Kurt Roth: Complex, Chaotic and Evolving Environmental Systems (Lecture Notes). Chapter 7.3 (Discrete Complex Systems - Contact Processes)
