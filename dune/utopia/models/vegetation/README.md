# A simple vegetation model

This is a very simple implementation of a vegetation model. It is implemented as a stochastic cellular automaton on a grid, where the state of each cell is a double scalar representing the plant bio-mass on that cell.

So far, the only driver for a change in the plant bio-mass is rainfall, implemented as a gauss-distributed random number drawn for each cell.

## Model parameters

* mean rainfall $`<r>`$
* rainfall standard deviation $`\sigma_r`$
* growth rate $`g`$
* seeding rate $`s`$

## Growth process

In each time step, the plant bio-mass on a cell is increased according to a logistic growth model. Let $`m_{t,i}`$ be the plant bio-mass on cell $`i`$ at time $`t`$ and $`r_{t,i}`$ the rainfall at time $`t`$ onto cell $`i`$. The plant bio-mass at time $`t+1`$ is then determined as

$`m_{t+1,i} = m_{t,i} + m_{t,i} \cdot g \cdot (1 - m_{t,i}/r_{t,i} `$.

## Seeding process

Since logistic growth does never get started if the initial plant bio-mass is zero, a seeding process is included into the model. If $`m_{t,i} = 0`$, the plant bio-mass at time $`t+1`$ is then determined as

$`m_{t+1,i} = s \cdot r_{t,i}`$.

## Behaviour for the default parameters

The current default parameters (as defined in the file `vegetation_cfg.yml`) are:

* $`<r> = 10`$
* $`\sigma_r = 2`$
* $`g = 0.1`$
* $`s = 0.2`$

For these parameters and a grid size of 10x10 the system takes roughly 50 time steps to reach a dynamic equilibrium, in which the plant bio-mass on all cells fluctuates around $`9.5`$.
