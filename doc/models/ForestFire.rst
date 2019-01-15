
``ForestFire`` model
====================

The ForestFire model simulates the development of a forest under influence of forest fires. Trees grow on a random basis and fires cause their death, for a whole cluster instantaneously (two state model).

Fundamentals
------------

The model is based on the description in the CCEES Lecture Notes by Kurt Roth in chapter 7.3 (Discrete Complex Systems - Contact Processes).

Subchapter of the Fundamentals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The two state forest fire. Available states for a cell in a cellular automaton are ``empty`` and ``tree``. Trees grow on an empty cell with probability ``growth_rate``. With the probability ``lightning_frequency`` a cell is ignited and this cell, as well as all connected cells (the cluster) burn instantaneously and turn to empty - a percolation occurs.

Implementation Details
----------------------

There is the possibility to use the model feature ``ignite_bottom_row`` instead of a lightning frequency. Cells don`t ignite at random, but any tree on the bottom (southern) row is ignited.

The model can be used as a percolation model if the simulation is run for a single time step.

Simulation Results – A Selection Process
----------------------------------------

See CCEES Lecture Notes

References
----------


* 
  Bak, P., K. Chen and C. Tang, 1990: A forest-fire model and some thoughts on turbulence, Phys. Lett. A, 147, (5-6), 297–300, doi: 10.1016/0375–9601(90)90451–S.

* 
  Kurt Roth: Complex, Chaotic and Evolving Environmental Systems (Lecture Notes). Chapter 7.3 (Discrete Complex Systems - Contact Processes)
