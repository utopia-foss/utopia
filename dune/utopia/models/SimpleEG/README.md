# Simple Evolutionary Games

_WIP_

This is a model of simple evolutionary games (on regular grids). It is in its core based on an extended and slightly modified version of Nowak & May 1992.

## Basics

In this model, the world is a (regular) grid of cells. Each cell represents one player<sup>1</sup> that possess two internal properties: a strategy and a payoff. The strategy - in the following denoted by $S_0$, $S_1$ determines how the player interacts with its neighbors. The interaction is pair-wise, meaning that a player interacts only with one other player in one game. However, the neighborhood of a player consits of multiple players, such that that per iteration step multiple games are played. The resulting gain from the individual games are summed up and stored as the player's payoff.

### Payoff From a Single Interaction

The gain from a single game is given by the interaction matrix:
$$
W = \bordermatrix{~ & S_0 & S_1 \cr
                  S_0 & w_{S_0S_0} & w_{S_0S_1} \cr
                  S_1 & w_{S_1S_0} & w_{S_1S_1} \cr}
$$
$S_0$ and $S_1$ are the two different strategies.<sup>2</sup> The matrix entries $w_{ij}$ define the payoff a player with strategy $S_i$ receives from a game played against a player with strategy $S_j$.

### Neighborhood

The neighbors of a player are defined by the Moore neighborhood (9-neighborhood) of a cell on the grid. Thus, every player has $9$ neighboring players. Thus, every player plays against 9 other players per iteration step.
<!-- Adapt this, if we include the VanNeumann neighborhood at some point.--> 
<!-- Include a figure of and reference to the Moore Neighborhood here? --> 

### Evolution

Evolution is modelled by a very simplified imitation update scheme. After the games have been played, every player looks in her neighborhood for the player with the highest payoff, thus the player with the most succesfull strategy in her corresponing neighborhood. If this highest payoff is even higher than the player's payoff, she adapts her strategy to the more successfull one. This means, in the next round, the player will participate in all the individual games with the strategy that yielded the best results in the previous round. 

### Iteration step



<sup>1</sup> In evolutionary game theory models, interacting entities are normally called players. This model is, at its current state, a pure evolutionary game model, therefore we will speak of players. However, if more complexity is introduced, e.g. by coupling this model with other model's the entities should be referred to as the more general agents.
<!-- Do we want to have this footnote? If yes, smaller font for footnotes? Perhaps link to agent definition in Kurt's Script? --> 
<sup>2</sup> In the case of a Prisoner's Dilemma these strategies correspond to cooperation ($S_0 = C$) and defection ($S_1 = D$). Here, we want to be able to model all kind of two-agent two-dimensional games. That's why we chose the more general nomenclature.

### Dive Into More Theory

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