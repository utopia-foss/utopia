
``Vegetation`` - A simple vegetation model
==========================================

This is a very simple implementation of a vegetation model. It is implemented as a stochastic cellular automaton on a grid, where the state of each cell is a double scalar representing the plant bio-mass on that cell.

So far, the only driver for a change in the plant bio-mass is rainfall, implemented as a gauss-distributed random number drawn for each cell.

Model parameters
----------------


* ``rain_mean``\ : mean rainfall :math:`\langle r \rangle`
* ``rain_std``\ : rainfall standard deviation :math:`\sigma_r`
* ``growth_rate``\ : growth rate :math:`g`
* ``seeding_rate``\ : seeding rate :math:`s`

Growth process
--------------

In each time step, the plant bio-mass on a cell is increased according to a logistic growth model. Let :math:`m_{t,i}` be the plant bio-mass on cell :math:`i` at time :math:`t` and :math:`r_{t,i}` the rainfall at time :math:`t` onto cell :math:`i`. The plant bio-mass at time :math:`t+1` is then determined as

:math:`m_{t+1,i} = m_{t,i} + m_{t,i} \cdot g \cdot (1 - m_{t,i}/r_{t,i})`.

It is possible that the result yields a negative value. In this case, the
population density is silenty set to zero, :math:`m_{t+1,i} = 0`.

Seeding process
---------------

Since logistic growth does never get started if the initial plant bio-mass is zero, a seeding process is included into the model. If :math:`m_{t,i} = 0`, the plant bio-mass at time :math:`t+1` is then determined as

:math:`m_{t+1,i} = s \cdot r_{t,i}`.

Behaviour for the default parameters
------------------------------------

The current default parameters (as defined in the file ``vegetation_cfg.yml``\ ) are:


* :math:`\langle r \rangle = 10`
* :math:`\sigma_r = 2`
* :math:`g = 0.1`
* :math:`s = 0.2`

For these parameters and a grid size of :math:`10 \times 10` the system takes roughly :math:`50` time steps to reach a dynamic equilibrium, in which the plant bio-mass on all cells fluctuates around :math:`9.5`.
