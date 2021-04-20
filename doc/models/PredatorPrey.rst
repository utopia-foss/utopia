.. _model_PredatorPrey:

``PredatorPrey`` Dynamics
=========================

This model is implemented as a cellular automaton (CA) with the cells arranged on a two-dimensional grid and represents a simple case of spatially resolved population dynamics.

.. note::

    Currently, the execution order of rules is different from the code referred to in the script.
    There, cells are called randomly and the sequence of rules is applied to an individual cell before proceeding to the next one.
    In the Utopia ``PredatorPrey`` model, a rule is applied to all cells before the proceeding to the next rule.
    Due to that, results from the script can not be replicated exactly with this code.

Scenario
--------
As the name suggests, there are two different species present in this model: prey and predator.
The prey species has a steady inflow of resources (e.g. by eating plants, whose population dynamics are not represented in this model).
The predator species feeds on the prey.
Both species expend resources to uphold their structure ("living costs") and to reproduce, which happens in an asexual manner (i.e. individuals can reproduce without a mate).

The interaction consists of the predator moving on the grid and looking for prey in its neighborhood. Upon making contact, it consumes the prey.
The prey may flee with a certain probability.

Implementation
--------------
This is modelled using a cellular automaton (CA). Each cell of the CA has four possible states:

* Empty
* Inhabited by a prey
* Inhabited by a predator
* Inhabited by both a prey and a predator

No two individuals of the same species can be on the same cell at the same time. Consequently, each cell contains a variable for each species, in which the resource level of the respective individual is stored.
The interaction is calculated for each timestep and consists of four sequentially applied rules:

#. **Cost:** resources of each individual are depleted by the cost of living.
   Individuals with negative or zero resources die and are hence removed.
#. **Movement:** predators move to a cell populated by prey in their
   neighborhood, or to an empty cell if there is no prey.
   Prey that are on a  cell together with a predator flee to an empty cell in their neighborhood with a certain probability. If there are several cells in the neigborhood that meet the above condition, one is chosen at random.
#. **Eating:**: prey consume resources and predators eat prey if they are on
   the same cell.
#. **Reproduction:** if an individual's resources exceed a certain value and
   if there is a cell in its neighborhood that is not already populated by an
   individual of the same species, it reproduces and an individual of the same
   species is created on the empty cell. 2 resource units are transferred to
   the offspring.

All cells are updated asynchronously.
The order for the cell update is random for rules 2 and 4, to avoid introducing any ordering artefacts. For rules 1 and 3, this is not required.

Initialization
--------------
Cells are initialized in a random fashion:
depending on the probabilities configured by the user, each is initialized in one of the four states. The parameters controlling this are given in the model configuration, listed below.


Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``PredatorPrey`` model:

.. literalinclude:: ../../src/utopia/models/PredatorPrey/PredatorPrey_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``PredatorPrey`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/PredatorPrey/PredatorPrey_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/PredatorPrey/PredatorPrey_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :ref:`utopya_base_cfg`.


References
----------
Kurt Roth: *Chaotic, Complex, and Evolving Environmental Systems*, unpublished lecture notes, University of Heidelberg, 2019.
