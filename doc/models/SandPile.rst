
``SandPile`` Model
==================

The ``SandPile`` model simulates the development of a sand pile when you add a grain of sand at each time step.
There is a critical level for the slope of the sandpile that causes it to relax and distibute grains on the surrounding cells.

The model is based on the description in the CCEES Lecture Notes by Kurt Roth in chapter 7.3 (Discrete Complex Systems - Contact Processes). See there for an in-depth theory and simulation results.


Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``SandPile`` model.

.. literalinclude:: ../../src/models/SandPile/SandPile_cfg.yml
   :language: yaml
   :start-after: ---

References
----------

* Bak, P., K. Chen and C. Tang, 1990: Self-organized criticality: An explanation of the 1/f noise, Phys. Rev. Lett., 59, 4, 381–384, doi: 10.1103/PhysRevLett.59.381.
* Kurt Roth: Complex, Chaotic and Evolving Environmental Systems (Lecture Notes). Chapter 7.2 (Discrete Complex Systems - Contact Processes)
