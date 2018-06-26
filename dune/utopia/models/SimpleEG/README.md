# Simple Evolutionary Games

_WIP_

This is a model of simple evolutionary games (on regular grids). It is fundamentally based on an extended and slightly modified version of Nowak & May 1992.

## Basics

### Theory

#### Two-Agent Games in 2D Strategy space: Game classification
Two-agent interactions with two possible outcomes can be classified into one of four different games according to Friedman & Barry 2016 depending on their interaction matrix. A general interaction matrix is given by:
$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$
It is useful to define the quantity $a_x$ (with $x \in \{ 0, 1\} in 2D), which describes the payoff advantage of $S0$ over $S1$ in a game against $S1$:
$$
w_x = w_{S_xS_x} - w_{S_x\bar{S_x}}
$$
where $\bar{S_x}$ is the complementary strategy of $S_x$.
From a more biological perspective, it can be interpreted as the fitness advantage of rare mutant strategy $S0$ if $S1$ is the common strategy.

_NOTE_: Its quite difficult to adapt the notation of Friedman&Barry2016 to our nomenclature - especially if one keeps in mind, that it would be nice to have it easily expandable for n-dimensions. Needs perhaps some discussion. 

##### Typ 1: Hawk-Dove game (HD game)
$w_1, w_2 >0$

##### Typ 2: Coordination game (CO game)
$w_1, w_2 <0$
##### Typ 3: Dominant Strategy game (DS game)
$w_1, < 0 < w_2 $ or $w_2, < 0 < w_1 $

Two types:
###### Typ 3a: Prisoner's Dilemma (PD Game)
mean fitness $\bar{w}$ decreases with increasing frequency of the dominant strategy. --> social dilemma
###### Typ 3b: Lasséz-Faire game (LF Game)
mean fitness $\bar{w}$ increases with increasing frequency of the dominant strategy. --> no social dilemma

## Implementation

- general interaction matrix, providable in three different forms:
If $b$ is given:
$$
W = \bordermatrix{~ & S_0=C & S_1=D \cr
                  S_0=C & 1 & 0 \cr
                  S_1=D & b & 0 \cr}
$$
If a benefactor-cooperator pair is given:
$$
W = \bordermatrix{~ & S_0=C & S_1=D \cr
                  S_0=C & b-c & -c \cr
                  S_1=D & b & 0 \cr}
$$

$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$

- differences to Nowak & May: no self-interactions -> take care with parameter comparison

## References
- Nowak, Martin A. & May, Robert M. _Evolutionary games and spatial chaos_, Nature, 1992
- Friedman, Daniel & Barry, Sinervo _Evolutionary games in natural, social, and virtual worlds_, Oxford University Press, 2016 