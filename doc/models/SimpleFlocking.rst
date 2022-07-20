.. _model_SimpleFlocking:

``SimpleFlocking`` Model
========================

The ``SimpleFlocking`` model implements one of the earliest models of collective dynamics that shows a phase transition.
It was proposed by Viczek *et al.* in 1995 (see :ref:`SimpleFlockingRefs`) and is inspired by the swarm behavior of schools of fish or flocks of bird (hence the name).


Model Fundamentals
------------------
Each agent in the ``SimpleFlocking`` model is characterized by its position in space, its orientation, and a (constant) speed.

The model mechanisms are very simple: In each time step, ...

1. ... all agent's orientations are adjusted to the mean value of those agents within an interaction radius (including the agent itself), plus some angular noise.
2. ... all agent's positions are updated using the displacement vector that results from the change in orientation.

These updates occur synchronously for all agents.


Implementation Details
----------------------
Observables
^^^^^^^^^^^
* ``agent``: group that contains agent-specific data, labelled by ``time`` and agent ``id``

    * ``x`` and ``y``: agent position
    * ``orientation``: agent orientation in radians :math:`[-\pi, +\pi)`

* ``norm_group_velocity``: the normalized average group velocity time series.
  This is what Vicsek et al., :ref:`1995 <SimpleFlockingRefs>`, refer to as the **order parameter** :math:`v_a` of the system.
* ``orientation_circmean``: circular mean orientation time series
* ``orientation_circstd``: circular standard deviation of orientation time series

Note the need for `circular statistics <https://en.wikipedia.org/wiki/Directional_statistics>`_ in order to allow calculating these observables.






Default Model Configuration
---------------------------
Below are the default model configuration parameters for the ``SimpleFlocking`` model:

.. literalinclude:: ../../src/utopia/models/SimpleFlocking/SimpleFlocking_cfg.yml
   :language: yaml
   :start-after: ---


Configuration Sets
^^^^^^^^^^^^^^^^^^
The following configuration sets aim to reproduce some of the results of the Vicsek *et al.*, :ref:`1995 <SimpleFlockingRefs>`, paper.
The parameters are those used in the referenced figures in that paper.

* ``flocking``: A scenario where the agents show flocking behavior (Figure 1b).
* ``ordered``: A scenario leading to a globally ordered movement (Figure 1d).
* ``noise_sweep``: A sweep that aims to reproduce Figure 2a: the order parameter over the system's noise amplitude for different system sizes.
* ``density_sweep``: A sweep that aims to reproduce Figure 2b: the order parameter over the agent density.

The config sets with sweeps also include the corresponding plot configurations to generate the relevant figure.


Plots
-----
The following general plots are available for the ``SimpleFlocking`` model:

* ``agents_in_domain``: shows the agents with color-coded orientation moving in the domain
* ``time_series``:

    * ``norm_group_velocity``: The normalized group velocity time series, which is used as the **order parameter** :math:`v_a` by Vicsek *et al.*.
    * ``orientation_circstd``: The circular standard deviation of the agents' orientation over time, which can also be interpreted as an order parameter.
    * ``orientation``: A combined plot of the circular mean and circular standard deviation of the agent orientation over time.

For details on how to further configure these, see the plot configurations.

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. raw:: html

   <details>
   <summary><a>See the default plot config</a></summary>

.. literalinclude:: ../../src/utopia/models/SimpleFlocking/SimpleFlocking_plots.yml
   :language: yaml
   :start-after: ---

.. raw:: html

   </details>

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. raw:: html

   <details>
   <summary><a>See the base plot config</a></summary>

.. literalinclude:: ../../src/utopia/models/SimpleFlocking/SimpleFlocking_base_plots.yml
   :language: yaml
   :start-after: ---

.. raw:: html

   </details>

For the utopya base plots, see :ref:`utopya_base_cfg`.


Possible Future Extensions
--------------------------
This model is really only a starting point in the interesting field of collective dynamics.
There are a number of interesting extensions, specifically when giving the individual agents more perceptive capabilities and possible actions.

In the work by Couzin *et al.*, :ref:`2002 <SimpleFlockingRefs>`, the agents adjust their movement as a result to "social forces": repulsion and attraction to other agents.
In addition, the agents' field of view is modelled and plays a role in the computation of these forces.
Many models include these sets of social forces and expand on that, see e.g. Twomey *et al.*, :ref:`2020 <SimpleFlockingRefs>`.

In the model by Klamser & Romanczuk, :ref:`2021 <SimpleFlockingRefs>`, a continuous version of these forces is used and a predator is introduced; this setup is then used to assess the system's ability to be in a self-organized critical state, maximizing sensitivity to the predator.
Furthermore, the agent neighborhood is determined via `Voronoi tesselation <https://en.wikipedia.org/wiki/Voronoi_diagram>`_ and some agent parameters are subject to evolutionary dynamics.



.. _SimpleFlockingRefs:

References
----------

* Vicsek, T., Czirók, A., Ben-Jacob, E., Cohen, I., & Shochet, O. **(1995)**. *Novel Type of Phase Transition in a System of Self-Driven Particles*. In Physical Review Letters (Vol. 75, Issue 6, pp. 1226–1229). American Physical Society (APS). `10.1103/physrevlett.75.1226 <https://doi.org/10.1103/physrevlett.75.1226>`_

* Couzin, I. D., Krause, J., James, R., Ruxton, G. D., & Franks, N. R. **(2002)**. *Collective Memory and Spatial Sorting in Animal Groups*. In Journal of Theoretical Biology (Vol. 218, Issue 1, pp. 1–11). Elsevier BV. `10.1006/jtbi.2002.3065 <https://doi.org/10.1006/jtbi.2002.3065>`_

* Twomey, C. R., Hartnett, A. T., Sosna, M. M. G., & Romanczuk, P. **(2020)**. *Searching for structure in collective systems*. In Theory in Biosciences (Vol. 140, Issue 4, pp. 361–377). Springer Science and Business Media LLC. `10.1007/s12064-020-00311-9 <https://doi.org/10.1007/s12064-020-00311-9>`_ (`PDF <https://link.springer.com/content/pdf/10.1007/s12064-020-00311-9.pdf>`_).

* Klamser, P. P., Romanczuk, P. **(2021)**. *Collective predator evasion: Putting the criticality hypothesis to the test*. PLOS Computational Biology 17(3): e1008832. `10.1371/journal.pcbi.1008832 <https://doi.org/10.1371/journal.pcbi.1008832>`_
