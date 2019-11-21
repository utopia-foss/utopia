``PredatorPreyPlant`` — Model of Predator-Prey Dynamics including Vegetation
============================================================================

This is an expansion of the :doc:`Predator-Prey <PredatorPrey>` model where plants (the resource the prey feeds on) is modelled explicitly.

The main additions/changes are the following:

* The movement mechanism changed and the movement rule now involves a movement radius so that each entity can move more than one cell per turn.
* The presence of plants which can grow according to different growth models

 
Implementation Details
----------------------
Movement
^^^^^^^^
In contrast to the ``PredatorPrey`` model, the movement rule is no longer called once per cell each turn.
Instead, it is called for ``num_moves`` random cells each turn.
This makes it possible that one cell can be selected multiple times to carry out the movement rule and other cells might not be selected at all within that time step.

The internal ``num_moves`` parameter can be specified in the configuration only via the ``num_moves_fraction``, which allows to specify it in units of the total number of grid cells.

Three possible actions take place on a selected cell:

* In case there are both predator and prey on a cell, the prey will flee in the same fashion as in the Predator-Prey model.
* If there is a predator but no prey on the cell, the predator will move until it finds a prey or until it reaches the ``move_limit``.
* If there is only a prey and no resource, the prey will move to find resources until it reaches it or arrives at the ``move_limit``. In this phase the prey will walk on a cell even if there is a predator on it.


Vegetation Growth Model
^^^^^^^^^^^^^^^^^^^^^^^
There are three options for the growth models of the vegatation:

* ``none``: deactivates explicit modelling of vegetation. Grass regrows every turn, thus preys can also eat every turn.
* ``deterministic``: vegetation will regrow after a specific number of time-steps.
* ``stochastic``: vegetation will regrow with a certain probability, evaluated each time step.


Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``PredatorPreyPlant`` model.

.. literalinclude:: ../../src/utopia/models/PredatorPreyPlant/PredatorPreyPlant_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``PredatorPreyPlant`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/PredatorPreyPlant/PredatorPreyPlant_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/PredatorPreyPlant/PredatorPreyPlant_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :doc:`here </frontend/inc/base_plots_cfg>`.


References
----------
Kurt Roth: *Chaotic, Complex, and Evolving Environmental Systems*, unpublished lecture notes, University of Heidelberg, 2019.
