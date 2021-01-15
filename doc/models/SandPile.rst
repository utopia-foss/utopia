.. _model_SandPile:

``SandPile`` Model
==================

The ``SandPile`` model simulates the development of a sand pile to which a grain of sand is added at each time step. At a critical slope level, the sandpile relaxes and distributes grains onto the surrounding cells.

The model is initialized with a random distribution of sand, with the slope exceeding the critical value everywhere.
It is then relaxed (sand topples) such that the model ends the initial iteration step with a slope below the critical slope everywhere. In each further iteration, a grain of sand is added at a random position. If the slope exceeds the critical value, sand topples to the neighboring positions. If this leads to a neighboring position exceeding the critical slope, the sand topples again. This goes on during each timestep until the slope is below the critical value everywhere.
The borders of the model's grid are fixed at a given (sub-critical) slope value, such that each sand may "fall off" the borders of the grid.

In each timestep, the rearrangement of grains can affect any number of positions. It is possible that the slope only changes in one position, or that a single grain added causes an avalanche that affects almost the entire grid.
These features of unpredictability and lack of characteristic scale make the model interesting; it is based on the description in the CCEES Lecture Notes by Kurt Roth in chapter 7.2 (Discrete Complex Systems – Sandpile Model). See there for an in-depth theoritcal description and simulation results.


Default Model Configuration
---------------------------
Below are the default configuration parameters for the ``SandPile`` model:

.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_cfg.yml
   :language: yaml
   :start-after: ---

Available Plots
---------------
The following plot configurations are available for the ``SandPile`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/SandPile/SandPile_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :ref:`utopya_base_cfg`.

References
----------

* Bak, P., K. Chen and C. Tang, 1990: Self-organized criticality: An explanation of the 1/f noise, Phys. Rev. Lett., 59, 4, 381–384, doi: 10.1103/PhysRevLett.59.381.
* Kurt Roth: Complex, Chaotic and Evolving Environmental Systems (Lecture Notes). Chapter 7.2 (Discrete Complex Systems - Contact Processes)
