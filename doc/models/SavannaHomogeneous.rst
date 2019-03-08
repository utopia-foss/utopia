
``SavannaHomogeneous``
======================

Complex dynamics of savanna landscapes
--------------------------------------

The homogeneous savanna model simulates a savanna landscape - composed of Grass, savanna tree Saplings, savanna Trees, and Forest trees - in a well mixed situation with numerical representation of the percolation model.

Fundamentals
------------

The model is based on the following coupled equations

.. math::

   \dot{G} = \mu S + \nu T - \beta G T + \phi \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right) F - \alpha G F

.. math::

   \dot{T} = \omega \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right) S - \nu T - \alpha TF

.. math::

   \dot{F} = \left(\alpha \cdot (1-F) - \phi \left( G + \gamma_s S + \gamma_t T + \gamma_f F \right) \right) F

with growth rates :math:`\alpha` and :math:`\beta` for forest- and savanna-trees respecitvely, 
mortality rates :math:`\mu` and :math:`\nu` for sapling and savanna trees respectively, 
and two sigmoidal functions :math:`\omega \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right)` and :math:`\phi \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right)` describing the transition from sapling to savanna tree and the mortality rate of forest trees respectively.

The sigmoidal functions are a numerical description of a classical percolation model for fire or disease spread, which states, that above a certain threshold density connectivity of cells increase drastically and hence connected clusters have infinit extend, whereas below the cluster extend is limited to strongly localized connected domains.
In the case of savanna fire spread this translates to a sharp decrease in the probability to survive a fire. 
From ecology we know that :math:`\omega` and :math:`\phi` depend on whether the associated species burned during a fire event [Touboul, 2018], hence transiting from :math:`\omega_0` to :math:`\omega_1` (resp. :math:`\phi_0` to :math:`\phi_1`) at the threshold density of flammable species :math:`G,\ \gamma_s S,\ \gamma_t T,\ \gamma_f F` with :math:`\gamma_x` a fire propagation factor for a species relative to grass.

Implementation Details
----------------------

The equations are solved using the classical euler intergration scheme on a single state set.




Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``SavannaHomogeneous`` model.
They are chosen following Touboul [2018].

.. literalinclude:: ../../src/models/SavannaHomogeneous/SavannaHomogeneous_cfg.yml
   :language: yaml
   :start-after: ---

Simulation Results – A Selection Process
----------------------------------------

For a good reference see the article from Touboul et al. (2018).

More Conceptual and Theoretical Background
------------------------------------------

For full model details see the reference article from Touboul et al. (2018) and consider the following adaptation.

Theory of heterogeneous Savannas
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The model was adapted from Touboul (2018) in the way, that it is possible to be extended to a explicit representation of the fire spread in a percolation model (see SavannaHeterogeneous for full detail).
For this the proposed factor :math:`\gamma` - acting to the same extend on S and T, never on F - was replaced with species proper :math:`\gamma_S,\ \gamma_T`, and :math:`\gamma_F`. Indeed, :math:`\gamma_F = 0` and :math:`\gamma_S = \gamma_T = \gamma` directly relates the two models.
However, in the storyline is, that saplings and forest trees transition rates depend on whether the plant burned or not, therefore we consider it crutial, that it itself is indeed flammable, while Touboul  et al. assume, that savanna trees and their saplings propagate the fire to a certain extend :math:`\gamma` and never forest trees do so.

References
----------

* Touboul, J.D., Staver, A.C., Levin, S.A., 2018. On the complex dynamics of savanna landscapes. Proceedings of the National Academy of Sciences 115, E1336–E1345. https://doi.org/10.1073/pnas.1712356115
