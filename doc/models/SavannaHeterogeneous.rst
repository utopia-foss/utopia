
``SavannaHeterogeneous``
========================

Complex dynamics of savanna landscapes using an explicit percolation model
--------------------------------------------------------------------------

The heterogeneous savanna model simulates a savanna landscape - composed of Grass, savanna tree Saplings, savanna Trees, and Forest trees - in a heterogeneous approach with explicit representation of the percolation model on a cellular automaton.

Fundamentals
------------

The dynamics in a well mixed situation can be described by the coupled equations [Touboul, 2018]

.. math::

   \dot{G} = \mu S + \nu T - \beta G T + \phi \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right) F - \alpha G F

.. math::

   \dot{S} = \beta G T - \left( \omega \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right) + \mu \right) S - \alpha G F

.. math::

   \dot{T} = \omega \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right) S - \nu T - \alpha TF

.. math::

   \dot{F} = \left(\alpha \cdot (1-F) - \phi \left( G + \gamma_s S + \gamma_t T + \gamma_f F \right) \right) F

with growth rates :math:`\alpha` and :math:`\beta`, and mortality rates :math:`\mu` and :math:`\nu` for sapling and savanna trees respectively, furthermore two sigmoidal functions :math:`\omega \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right)` and :math:`\phi \left(G + \gamma_s S + \gamma_t T + \gamma_f F \right)` describing the transition from sapling to savanna tree and the mortality rate of forest trees respectively.

The sigmoidal functions are a numerical description of a classical percolation model for fire or disease spread, which states, that above a certain threshold density connectivity of cells increases drastically and hence connected clusters have infinit extend, whereas below the cluster extend is limited to strongly localized domains.
In the case of savanna fire spread this translates to a sharp decrease in the probability to survive a fire. 
From ecology we know that \omega and \phi depend on whether the associated species burned during a fire event [Touboul, 2018], hence transiting from :math:`\omega_0` and :math:`\omega_1` (resp. :math:`\phi_0` and :math:`\phi_1`) at the threshold density of flammable species :math:`G,\ \gamma_s S,\ \gamma_t T,\ \gamma_f F` with :math:`\gamma_x` a fire propagation factor for a species relative to grass.

Here, we want to expand this model to a full cellular automaton description in heterogeneous space with explicit description of the percolation-like fire spread.
Therefore, instead of using the two sigmoidal functions :math:`\omega` and :math:`\phi` we perform a percolation from grass cells, ignited by lightning with frequency :math:`f` per cell, within a connected domain of flammable species :math:`G,\ \gamma_s S,\ \gamma_t T,\ \gamma_f F`, where :math:`\gamma` denotes the probability to catch fire from a neighboring cell.
The transition rates are then the two limits of the sigmoidal function :math:`\omega_0` and :math:`\omega_1` (resp. :math:`\phi_0` and :math:`\phi_1`).

All other rates can be directly applied to a cell of unique state (G, S, T, or F) in the form of conditional transition probabilities.

Furthermore, the growth process is localized, such that offspring plants (new forest trees, and saplings) grow according to a gaussian distribution with width :math:`\sigma` around existing parent trees. This translates to an effective (local) density :math:`T_\sigma(z)` and :math:`F_\sigma(z)`. Uniform growth rates is restored for :math:`\sigma` -> infinity (hard-coded at :math:`\sigma = 0` using global densities).

Implementation Details
----------------------

The translation to a CA
^^^^^^^^^^^^^^^^^^^^^^^

The dynamics of the dynamical system can be restored easily since all terms represent transitions from one species to another and are multiplicatives of a rate with one or two species densities.

Rates depending on the own density (e.g. :math:`\dot{T} = - \nu T` with corresponding :math:`\dot{G} = \nu T`) can be directly translated to a probability evaluation of the form:


* if cell has state T: pull random number :math:`p \in [0,1]`
* if further :math:`p < \nu`: change state T -> G

Rates depending on two densities (eg. :math:`\dot{T} = - \alpha T F`) the translation to CA is the following:


* if cell has state T: pull random number :math:`p \in [0,1]`
* if further :math:`p < \alpha F_\sigma(z)`: change state T -> F

using the effective density

.. math::

   T_\sigma (z) = \frac{\sum_{z'\mathrm{\ is\ tree}} \exp ( -(z-z')^2 / 2 \sigma^2)}
                       {\sum_{z'} \exp ( -(z-z')^2 / 2 \sigma^2}

at the cell's position z, which is the global density of F for a homogeneous representation, and depending on the number of forest trees within a certen distance for a heterogeneous representation. 

All terms of the four differential equations are represented of one of the two previous examples.

The time step
^^^^^^^^^^^^^

The time step splits in three distinct functions. 
The calculation of effective density, the update as a function of a cell's own state and its tags and the ignition of clusters in a percolation process.

The ignition returns a burned tag to the concernced cells, which is used in the consecutive step.

1. Calculation of effective density.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Trees are assumed to have finite reach to spread their seeds, hence new forest trees and saplings can grow only within a certain distance of a tree. (1) represents a convolution of the tree's CA with a gaussian of variance :math:`\sigma^2` and is performed as a multiplication in fourier space (using the Fastest Fourier Transform in the West, FFTW, library).

From this every cell becomes two effective density tags, one from savanna trees and one from forest trees.

2. Update
~~~~~~~~~

The cell's state is updated depending on its state and the given tags (effective densities and burned from previous step) in the way explained above.

3. Ignition
~~~~~~~~~~~

Cells with state Grass ignite with the probability f.
In a percolation like process the fire is spread instantaneously to all neighbors, however a neighboring cell catches fire with a certain probability depending on its state. Usually this probability is 1 for Grass, smaller 1 for Saplings and Forest and 0 for Trees (tbd by user). This process is repeated until each possible spread has been evaluated with the according probability.

Details
^^^^^^^

The equations above have been transformed to dimensionless equations, indeed this can be easily done, since all terms depend on a single rate and a dimensionless density.
As characteristic time is considered the characteristic lifetime of a savanna tree state (:math:`t_c = 1/\nu`). However, we stick to the parameter values given by Touboul (2018) and instead scale all equations with a time step resolution dt. All rates transform to a multiple of dt.

Simulation Results – A Selection Process
----------------------------------------

For a good reference see the article from Touboul et al. (2018).

More Conceptual and Theoretical Background
------------------------------------------

For full model details see the reference article from Touboul et al. (2018) and consider the following adaptation.

References
----------


* 
  Touboul, J.D., Staver, A.C., Levin, S.A., 2018. On the complex dynamics of savanna landscapes. Proceedings of the National Academy of Sciences 115, E1336–E1345. https://doi.org/10.1073/pnas.1712356115

* 
  Schertzer, E., Staver, A.C., Levin, S.A., 2015. Implications of the spatial dynamics of fire spread for the bistability of savanna and forest. Journal of Mathematical Biology 70, 329–341. https://doi.org/10.1007/s00285-014-0757-z
