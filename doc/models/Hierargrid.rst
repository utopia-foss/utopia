
``Hierargrid`` – Emergence of cooperation in spatial interactions
=================================================================

The ``Hierargrid`` has the same fundamental structure as the `Hierarnet <Hierarnet.html>`_ model 
with the main difference that the population is not prescribed by a network
but by a spatial grid. 

In the following, only differences to the ``Hierarnet`` model will be pointed out.


Environment
-----------
Each cell on which an agent located has a maximal capacity :math:`\kappa_{max}` 
that determines how much wealth :math:`W` can be extracted per time step. 
The actual payoff is weighted by a wealth weighting factor :math:`\kappa`:

.. math::

    \kappa = 1 - \frac{W}{\kappa_{max}}


Default configuration parameters
--------------------------------

Below are the default configuration parameters for the ``Hierargrid`` model.

.. literalinclude:: ../../src/models/Hierargrid/Hierargrid_cfg.yml
   :language: yaml
   :start-after: ---


Interaction Payoff
------------------
The ``Hierargrid`` model provides the possibility of accumulated payoffs.
This means that the gain of each time step is added to the previously already 
gained payoff. It resets to nothing only if the agent dies.