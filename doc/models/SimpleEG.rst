.. _model_SimpleEG:

``SimpleEG``: Evolutionary Games
================================

This is a model of simple evolutionary games (on regular grids). It is based on an extended and slightly modified version of Nowak & May (1992).

.. contents::
   :local:
   :depth: 2

----

Fundamentals
------------

The world is a regular square grid of cells. Each cell represents one player that possesses two internal properties: a strategy and a payoff. The strategy – in the following denoted by :math:`S_i`, where :math:`i` denotes a specific strategy from the set of all possible strategies – determines how the player interacts with its neighbors. Thus, the basic interaction is pair-wise.
However, the neighborhood of a player, which is defined through the grid, consists of multiple players, such that that multiple games are played per iteration step.
The resulting gain from the individual games are summed up and stored as each player's payoff.

.. note:: *Regarding the 'players' term:* in evolutionary game theory models, interacting entities are normally called 'players'. This model is, in its current state, a pure evolutionary game model, therefore we will also speak of "players". However, if more complexity is introduced, e.g. by coupling this model with other models, the entities should be referred to by the more general term "agents".

The Game – The Gain
^^^^^^^^^^^^^^^^^^^

The payoff a player receives from a single game is encoded in the interaction matrix:

.. math::

   W = \begin{pmatrix}w_{00} & w_{01} \cr w_{10} & w_{11} \end{pmatrix}.

The matrix element :math:`w_{ij}` denotes the payoff a player with strategy :math:`S_i` receives when playing against a player with strategy :math:`S_j`.
The two possible strategies in these games are :math:`S_0` and :math:`S_1`.

.. note:: In the case of a Prisoner's Dilemma, these strategies correspond to cooperation (:math:`S_0 = C`) and defection (:math:`S_1 = D`). Here, we want to be able to model all kind of two-dimensional two-player games, hence the more general nomenclature.

The Neighborhood – Just Look Around
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The neighbors of a player placed on a regular square grid are determined by the Moore neighborhood (9-neighborhood) of the corresponding cell. Thus, every player has 9 neighbours against which she plays in a single iteration step.


Evolution – Simply the Best
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Evolution is modelled by a very simple imitation update scheme. After all games are played, every player looks in her neighborhood for the player with the highest payoff, thus, the player with the most successful strategy in her corresponding neighborhood. If this highest payoff is higher than the player's payoff, she adapts her strategy to that of the more successful neighbor. This means that, in the next round, the player will participate in all the individual games with the strategy that locally yielded the best result in the previous round.

If more than one player has the highest fitness in a neighborhood, one of these is selected randomly to pass on her strategy.

Iteration Step – Time in Steps
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

An interaction step consists of two stages:


#. **Play the games:** every player plays with all players in her neighborhood
   the game defined by the interaction matrix. The payoffs of the games are
   summed up and stored as a property of the players.

#. **Update the strategies:** each player looks within its neighborhood for
   the most successful strategy (highest payoff) and adapts this strategy for
   the next iteration step.

These two steps are repeated for the desired number of iteration steps.

Implementation Details
----------------------

This section provides a few important implementation details.

Interaction: Differences to Nowak & May (1992)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In contrast to the implementation of Nowak & May (1992), this model excludes self-interactions. This means that a player does not play against herself in this model. This leads to difficulties in comparing the parameter regimes found by Nowak & May (1992) with the parameter regimes obtained in this model. However, qualitatively the same system regimes can be observed for slightly different parameters.

Interaction Matrix
^^^^^^^^^^^^^^^^^^

The interaction matrix that defines the interaction between the players can be provided in three different ways:

#. Setting the benefit ``b``: if we presuppose a simple Prisoner's Dilemma
   interaction, we can derive a complete interaction matrix just with one
   parameter :math:`b (>1)`, which determines the benefit a defector gets from
   playing against a cooperator. This simple parametrization is the one used
   in Nowak & May (1992).

#. Setting the benefit-cost pair ``bc-pair``: a Prisoner's Dilemma can also be
   defined by the two parameters :math:`b` and :math:`c` with :math:`b>c`.
   :math:`b` encodes the benefit a player gets from a cooperative opponent and
   :math:`c` encodes the cost a cooperator pays for cooperating with the
   opponent.

#. Setting a general interaction matrix ``ia_matrix``: the first two options
   only define different parameterizations of the Prisoner's Dilemma. However,
   it is possible to model all other possible linear two-strategy two-player
   interactions by setting all matrix elements explicitly. With the right
   choice of parameters, one can for example model a Hawk-Dove games, a
   Coordination Games, or a Laissez-Faire game. For more details on these games
   and their definitions see the theory section below.

The interaction matrices of the individual options are collected here:

.. list-table::
   :header-rows: 1

   * - via benefit ``b``
     - via benefit-cost-pair ``bc-pair``
     - via a general interaction matrix ``ia_matrix``
   * - :math:`\begin{pmatrix} 1 & 0 \\ b & 0 \end{pmatrix}`
     - :math:`\begin{pmatrix} b-c & -c \cr b & 0 \end{pmatrix}`
     - :math:`\begin{pmatrix}w_{00} & w_{01} \cr w_{10} & w_{11} \end{pmatrix}`


The algorithm is designed such that if an interaction matrix is provided in the configuration file the interaction matrix will define the game, even if ``b`` or ``bc-pair`` are also provided. If there is no interaction matrix, but a ``bc-pair`` provided, the interaction matrix will be derived from it, even if ``b`` is set. ``b`` is used only if none of the other options is provided.


Default Model Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below is the default model configuration for the ``SimpleEG`` model:

.. literalinclude:: ../../src/utopia/models/SimpleEG/SimpleEG_cfg.yml
   :language: yaml
   :start-after: ---



Simulation Results
------------------

*TODO*


More Conceptual and Theoretical Background
------------------------------------------

This section provides a deeper understanding of the underlying evolutionary game theory. It is useful to grasp the full capabilities of the *SimpleEG* model, e.g. which storylines can potentially be modelled. The theory is mainly taken from Friedmann & Barry (2016), if not otherwise indicated. The reader is referred to this book for a more detailed explanation of the theory. For a more basic and general introduction on evolutionary game theory, the reader is referred to Nowak (2006).


Linear Two-Player Games in 2D Strategy Space: Game classification
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

According to Friedman & Barry (2016), linear two-player, two-strategy interactions can be classified into one of four different games by their interaction matrix. A general interaction matrix has the form:

.. math::

   W = \begin{pmatrix}
   w_{00} & w_{01} \cr
   w_{10} & w_{11}
   \end{pmatrix}.

It is useful to define the quantity :math:`a_x` with :math:`x \in \{ 0, 1\}`, which describes the payoff advantage of :math:`S_0` over :math:`S_1` in a game against :math:`S_1` if the strategy space is two dimensional:

.. math::

   a_x = w_{S_x\bar{S_x}} - w_{\bar{S_x}\bar{S_x}},

where :math:`\bar{S_x}` is the complementary strategy of :math:`S_x`.
From a more biological perspective, it can be interpreted as the fitness advantage of rare mutant strategy :math:`S_0` if :math:`S_1` is the common strategy.

Type 1: Hawk-Dove game (HD game): :math:`a_0, a_1 >0`
"""""""""""""""""""""""""""""""""""""""""""""""""""""""

In this parameter regime, the game has the characteristics of a *Hawk-Dove game*. It is also known as the *game of chicken* or *snowdrift game*.
We describe its logic using the Hawk-Dove picture, although the same situation can be adapted to a multitude of situations that can be found in a multitude of areas, such as biology, or even international politics. This game was also used in the foundation of evolutionary game theory in the paper by May & Price (1973).

In a Hawk-Dove game, two players compete for a resource that cannot be split up. They can choose between two strategies: threat displays (Dove) or attack (Hawk). If both choose the Hawk strategy they attack each other and fight until one of them is injured and the other wins. If one player behaves as a Hawk and the other one as a Dove the Hawk defeats the Dove. If both players choose Dove, they both get a small benefit. However, the payoff is smaller than for a Hawk playing against a Dove.

If the parameters are chosen such that the relation :math:`a_0, a_1 >0` is met, the game is a version of a Hawk-Dove game. Varying the parameters varies the relation of the outcomes from the individual strategies in the games, however, their qualitative outcome does not change.

Type 2: Coordination game (CO game): :math:`a_0, a_1 <0`
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

This game is perhaps best explained using Jean-Jacques Rousseau's *Discourse on the Origin and Basis of Inequality Among Men* (1754):

..

   If it was a matter of hunting a deer, everyone well realized that he must remain faithful to his post; but if a hare happened to pass within reach of  one  of  them,  we  cannot  doubt  that  he  would  have  gone off in pursuit of it without scruple.


Extracting the underlying nature of the interaction: all players profit most if they work for a common aim, but single players can get distracted by more easily achievable goals with smaller returns. There is a steady state — all cooperating — which however is unstable to some degree.

Such situations can be modelled if the parameters of the interaction matrix obbey the relation :math:`a_0, a_1 <0`.

Type 3: Dominant Strategy game (DS game): :math:`a_0, < 0 < a_1` or :math:`a_0, > 0 > a_1`
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

In dominant strategy games, one strategy can be easily invaded by the other one, whereas the other cannot be invaded. Assuming replicator dynamics, this means that one strategy has a fixation, thus a population will evolve towards it.

There are two qualitatively different types of dominant strategy games:

Type 3a: Prisoner's Dilemma (PD Game): :math:`a_0, < 0 < a_1`
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

The Prisoner's Dilemma is arguably the most famous social dilemma. The standard story to explain the nature of the dilemma uses two prisoner's from the same criminal gang. Let us call them A and B. They are confined separately and cannot communicate with each other. Their sentencing is dependent on the decision of the other prisoner: if A and B betray each other, each serves two years in prison. If A betrays B and B remains silent, then A will be set free and B will need to serve three years in prison. If they both remain silent, they will receive a reduced sentence of one year. Therefore, the decision of one prisoner is dependent on the decision of the other one. Both face a social dilemma situation.

To put it in the standard parameter set, a two dimensional interaction matrix of the form

.. math::

   W = \begin{pmatrix}
   R & S \\
   T & P
   \end{pmatrix}

defines a Prisoner's Dilemma if the parameters have the following relation: T (temptation) > R (reward) > P (punishment) > S (sucker's payoff). This formulation is equivalent to the condition: :math:`a_0, < 0 < a_1`.

In a Prisoner's Dilemma the mean fitness :math:`\bar{w}` decreases with increasing frequency of the dominant strategy. This means that the more players defect, the lower the mean fitness gets.

Type 3b: Laissez-Faire game (LF game): :math:`a_1, < 0 < a_0`
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

In the Laissez-Faire game regime – defined by the relation :math:`a_1, < 0 < a_0` – there is no social dilemma any more. One strategy is dominant and also results in the highest mean payoff. The mean fitness :math:`\bar{w}` increases with increasing frequency of the dominant strategy.

Possible Future Extensions
--------------------------

This section collects ideas of generalizing and extending the model:

* **Generalize interactions to n-dimensional linear games:** currently, players can only choose between two different strategies. This could be generalized rather easily to *n* dimensions by generalizing the interaction matrix to *n* dimensions. With this, it would be possible to model for example rock-paper-scissors dynamics (:math:`n=3`).

References
----------

* Friedman, Daniel & Barry, Sinervo (2016). *Evolutionary Games in Natural, Social, and Virtual Worlds*. Oxford University Press.
* Nowak, Martin A. & May, Robert M. (1992). *Evolutionary Games and Spatial Chaos*. Nature
* Nowak, Martin A. (2006). *Evolutionary Dynamics*. Harward University Press.
* Rousseau, Jean-Jaque (1754). *Discourse on the Origin and Basis of Inequality Among Men*.
* Smith, Maynard J. & Price, G.R. (1973). *The logic of animal conflict*. Nature. doi:10.1038/246015a0.
