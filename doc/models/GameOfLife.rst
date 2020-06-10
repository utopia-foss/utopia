
``GameOfLife`` — Conway's Game of Life
======================================

This model implements `Conway's Game of Life <https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life>`_ as well as all two-dimensional `life-like cellular automata <https://en.wikipedia.org/wiki/Life-like_cellular_automaton>`_. 
For information on the model and its generalization to two-dimensional rules please have a look at the linked Wikipedia articles and, if needed, follow the references presented in the articles.

Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``GameOfLife`` model.

.. literalinclude:: ../../src/utopia/models/GameOfLife/GameOfLife_cfg.yml
   :language: yaml
   :start-after: ---

Available Plots
---------------
The following plot configurations are available for the ``GameOfLife`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/GameOfLife/GameOfLife_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/GameOfLife/GameOfLife_base_plots.yml
   :language: yaml
   :start-after: ---

For the utopya base plots, see :doc:`here </frontend/inc/base_plots_cfg>`.


Possible Future Extensions
--------------------------

This model can be expanded in many different ways. A few ideas are:

- Expand the initialization options to position well-known structures such as gliders or space-ships at desired locations on the grid
- Introduce stochasticity into the model by introducing birth and/or death probabilities. For these cases, also provide means to plot and analyze the data.

References
----------

* `Conway's Game of Life <https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life>`_
* `Life-like cellular automata <https://en.wikipedia.org/wiki/Life-like_cellular_automaton>`_
