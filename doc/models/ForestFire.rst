
``ForestFire`` Model
====================

The ``ForestFire`` model simulates the development of a forest under influence of fires and was first proposed by P. Bak, K. Chen, and C. Tang in 1990.
Trees grow on a random basis and lightning causes them and the trees in the same cluster to burn down instantaneously; this is the so-called "two state model".

.. contents::
   :local:
   :depth: 2

----

Implementation
--------------
The model is implemented based on the the description in the CCEES lecture notes by Kurt Roth (chapter: Discrete Complex Systems - Contact Processes).
The forest is modelled as a cellular automaton, where each cell has a state: ``empty`` or ``tree``.

The update procedure is as follows: In each iteration step, iterate over all cells in a random order. For each cell, one of two actions can take place, depending on the current state of the cell:

* An ``empty`` cell becomes a ``tree`` with probability ``p_growth``.
* A ``tree`` ignites with a probability ``p_lightning`` and ignites all cells indirectly connected to it. The cluster burns down instantaneously and all cells transition to state ``empty``.

The new state of a cell is applied directly after it was iterated over, i.e. cell states are updated *sequentially*.

Heterogeneities
^^^^^^^^^^^^^^^
There is the possibility to introduce heterogeneities into the grid, which are implemented as two additional possible cell states, ``source`` and ``stone``:

* A cell can be a constantly ignited fire ``source``, instantly burning down a cluster that comes in contact with it.
* A cell can be a ``stone``, being immune to fire and not taking part in any model dynamics.

These heterogeneities are controlled via the ``ignite_permanantly`` and ``stones`` entries of the model configuration, which both make use of the :ref:`entity selection interface <entity_selection>`.

Data Output
^^^^^^^^^^^
The following data is stored alongside the simulation:

* ``kind``: the state of each cell. Possible values:

   * ``0``: ``empty``
   * ``1``: ``tree``
   * ``2``: (not used)
   * ``3``: ``source``, is constantly ignited
   * ``4``: ``stone``, does not take part in any interaction

* ``age``: the age of each tree, reset after lightning strikes
* ``cluster_id``: a number identifying to which cluster a cell belongs; ``0`` for non-tree cells
* ``tree_density``: the global tree density


Simulation Results
------------------
Below, some example simulation results are shown.
If not marked otherwise, the default values (see below) were used and the grid was chosen to have :math:`1024^2` cells.

Spatial Representation
^^^^^^^^^^^^^^^^^^^^^^
Snapshots of the forest state, its age and the identified clusters after 223 iteration steps:

.. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/forest_snapshot.png
  :width: 600

.. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/forest_age_snapshot.png
  :width: 600

.. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/clusters_snapshot.png
  :width: 600

Tree Density
^^^^^^^^^^^^
The time development of the tree density shows how the system relaxes into its quasi-stable equilibrium state.

.. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/tree_density.png
  :width: 600

..
    (-- To be added once corresponding simulation have been run --)

    The temporal development of the mean tree density (sample size: 10) for five different lightning probabilites:

    .. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/tree_density_over_p_lightning.png
      :width: 600

    Additionally, we can look at the asymptotic tree density.
    Here, "asymptotic" refers to both time and space, as a small grid will show undesirable results.

    .. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/tree_density_asymptotic.png
      :width: 600

Cluster Size Analysis
^^^^^^^^^^^^^^^^^^^^^
In a complementary cumulative cluster size distribution plot, the scale-free nature of the system can be deduced by observation of those parts of the plot that appear linear in a log-log representation.

.. image:: https://ts-gitlab.iup.uni-heidelberg.de/utopia/doc_resources/raw/master/models/ForestFire/compl_cum_cluster_size_dist.png
  :width: 600

.. _FFM_cfg:

Default Configuration Parameters
--------------------------------
Below are the default configuration parameters for the ``ForestFire`` model.

.. literalinclude:: ../../src/utopia/models/ForestFire/ForestFire_cfg.yml
   :language: yaml
   :start-after: ---


Available Plots
---------------
The following plot configurations are available for the ``ForestFire`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/ForestFire/ForestFire_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/ForestFire/ForestFire_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :doc:`here </frontend/inc/base_plots_cfg>`.

References
----------

* Bak, P., K. Chen and C. Tang, 1990: A forest-fire model and some thoughts on turbulence, Phys. Lett. A, 147, (5-6), 297–300,
  doi: `10.1016/0375–9601(90)90451–S <https://doi.org/10.1016/0375-9601(90)90451-S>`_
  (`PDF <http://cqb.pku.edu.cn/tanglab/pdf/1990-55.pdf>`_).

* Kurt Roth: Complex, Chaotic and Evolving Environmental Systems (Lecture Notes). Chapter: Discrete Complex Systems - Contact Processes
