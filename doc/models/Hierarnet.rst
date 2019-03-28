
``Hierarnet`` — Model of Social Emergence of Cooperation
========================================================

*Note:* The Hierarnet model is described in more detail in :ref:`Herdeanu (2018) <literature-ref>`.

Abstract
--------

Cooperation is ubiquitous in nature. Its emergence can be a result of evolutionary processes through which single entities agglomerate to form hierarchically higher units. Examples of these transitions are genes that form genomes, single cells cooperating to form organisms, or individuals creating societies. Evolutionary game theory (EGT) has elucidated mechanisms that promote the evolution of cooperative behavior. However, most models presume discrete sets of agents’ strategies and do not account for the emergence of these strategies in the first place.

The *Hierarnet* model describes social interactions in structured populations based on a continuous and unrestricted cost strategy space. The interactions are based on an extended and generalized continuous public goods game and different population structures are provided by the underlying network topology. An evolutionary process introduces small random mutations in the agents’ contributions, thereby providing a natural way of modeling variation. Agents are selected for reproduction according to their success in the interactions. Among other phenomena, a dilemma regime in which agents have a disadvantage by cooperating with others is observed.
Simulation of the *Hierarnet* model shows that cooperative behavior can emerge and thrive in structured population although agents can harm themselves by contributing more than others.

Fundamental concepts
--------------------

My intention is to provide a rather general and abstract model and extract therefore rather general statements out of it. In future work, it could be adapted to more specific storylines. However, in the current model, agents are abstract objects, that can potentially represent a huge variety of entities, such as for example humans, animals, plants, but also ideas, information, public opinion in a certain sense even technological or cultural entities. Agents represent any entities that are 

#. connected to each, creating a social population structure, and 
#. interact with each other through a mechanism in which an entity pays a personal cost to create a benefit for itself and other entities. 

Hereby, costs and benefits can represent among others energy, money, influence, or time. The abstract concept of an agent and the abstractness of the results should be kept in mind.

Structure and Dynamics
----------------------

Imagine a network of agents located on its nodes. They interact with each other through a modified and expanded version of the public goods game. They evolve their costs through an evolutionary process. 
The dynamics can be split up into three different steps: initialization, interaction, and evolution. The model is initialized at the beginning of the simulation. Afterwards, all agents first interact with each other. Then, one agent is selected according to its fitness for reproduction, creating an offspring with an inherited, slightly mutated cost trait. From another, equivalent, perspective, a strategy is simply adapted by an agent via looking at a successful neighboring strategy. The strategy is copied with a small mutation error. Strategy in this context means cost. The reader should in the following always keep in mind, these different perspectives, describing the same mechanism.
Interaction and evolutionary process are repeated for a given number of iteration steps. In the following sections, the single processes will be explained in more detail.

Initialization
^^^^^^^^^^^^^^

In the first step, the model is initialized. This means that all agents are set up on the nodes of a network and interconnected via links. An individual agent’s neighborhood is determined by the chosen network topology; with it, the social structure of the population is provided. Further, each agent’s individual cost is set to zero. Thus, agents "interact neutrally" with each other.

Interaction
^^^^^^^^^^^

For an unstructured, well-mixed population of :math:`N` agents, the total wealth produced in an expanded version of the continuous public goods game is given by:

.. math::

   W = \sum_{i=1}^N r c_i = r \sum_{i=1}^N c_i

with :math:`c_i \in (-\infty , \infty) \forall i` being the agents’ costs, and 𝑟 being the synergy factor that determines how much wealth of the public good is created from a given invested cost. This wealth can be seen as a gross wealth, where the values of the agents’ cost contributions are not yet subtracted. It is equally shared among the population such that an agent :math:`a` gets an individual net payoff:

.. math::

   P_a = W/N - c_a = r/N \sum_{i=1}^N c_i - c_a

with :math:`c_i, c_a \in (-\infty , \infty) \forall i,a`. Here, the payoff represents the net benefit an agent receives from the interaction, meaning the share from the created ”group wealth” minus its individual cost for creating its part of the shared wealth.

Agents are located on a network. That means that in each round they participate in multiple games, namely the one centered around them and the ones centered around their neighbors. The total payoff an agent receives in one interaction step is the summed up payoff from the individual subgames. Further, the cost is split up among the individual subgames each agent participates (in the normalized interaction mode). In total, this results in the total payoff an agent on a network receives:

.. math::

   \Pi_a = \sum_{l \in N_a} \left[ \frac{r}{n_l} \sum_{j \in N_l} \frac{c_j}{n_j} \right] - c_a

with :math:`N_x` being the set of neighbors of agent :math:`x` and :math:`n_x = |N_x|`. For a detailed derivation of this formula see :ref:`Herdeanu (2018) <literature-ref>`.

This continuous version of a Public Goods Game follows the framework of a classical Public Goods Game because the agents create public goods or rather resources from which everybody in their interacting group profits. All agents are given the ability to interact with each other in a cooperative way, but with ever decreasing costs, they can create a more and more selfish and defective population. On the contrary, with rising costs, they can create a more and more cooperative population.

Cost can also be negative. At first sight this seems odd because there presumably are no such things as zero cost. However, in the social interaction context it makes sense as probably is best explained using the example of clean air. Clean air is a public good because everybody needs it and profits if it is present. People can create clean air by investing costs to filter out impurities or they can actively pollute the air for example by driving a car. In the context of the here presented version of the Public Goods Game the former corresponds to a positive cost while the latter to a negative one. This way of interpretation can again be applied to a multitude of storylines.

Due to no restrictions on the cost values, Red Queen dynamics can be expected, meaning that for rising costs, benefactors would need to contribute more and more to stay benefactors. Hence, in the current state, the *Hierarnet* model can mainly be used to investigate whether populations develop towards more or less cooperative behavior.

Darwinian Evolution
^^^^^^^^^^^^^^^^^^^

Initially, the population of agents is neutral, meaning that the agents contribute no cost. This changes through an evolutionary process, based on the three necessary mechanisms variation, selection, and heredity. 

This evolutionary process is implemented using a death-birth process. In each iteration step an agent is selected randomly from the whole population to die. It's neighbor compete for the vacancy such that one neighbor is selected fitness-dependent to create an offspring on the vacant node, which inherits the neighbors cost with a small random mutation. Hereby, fitness is equal to payoff, thus, success in the interactions within the social surrounding.

The fitness-dependent selection of the neighbor can be either deterministic or stochastic. In the deterministic case the fittest, thus, most successful neighbor is always selected to spread its strategy by creating an offspring. In the stochastic case there is a chance that errors occur, meaning that less fit neighbors occasionally get selected to reproduce. The probability to get selected is hereby proportionally to the exponential payoffs. Hence, the fittest neighbor within a neighborhood will still be selected most of the times.

For a more detailed description see :ref:`Herdeanu (2018) <literature-ref>`.

Counteracting Pressures
^^^^^^^^^^^^^^^^^^^^^^^

The Darwinian evolutionary process introduces a fitness-dependent selection towards the fitter, more-successful agents. Due to the random mutations the system can self-evolve either towards a more cooperative or a more defective population. However, due to the randomness of mutations, the evolution’s direction is not prescribed, but rather an emergent effect out of the fitness-dependent selection. In constructing the model, I expected two different kinds of selection pressures, counteracting each other:


#. From the individual agent’s perspective, investing a high cost results in an evolutionary disadvantage compared to its social neighborhood. It is a direct consequence of the underlying interaction. From this, evolution should be expected to select for selfish, increasingly defective agents.
#. From the point of view of an agent’s social neighborhood, every member of it gets fitter if an agent within the social subgroup increases its cost. Thus, on the level of social neighborhoods, a group of connected cooperative agents has an evolutionary advantage over a group of connected defective agents. Therefore, evolution could also be expected to select for more cooperative agents. A necessary prerequisite for this is to have some kind of group forming. This could be provided by separate, interacting groups, spatiality or an internal population structure.

The intensity of the two different competing selection pressures is expected to be mainly dependent on the synergy factor, thus by the amount of created wealth, and the population structure, provided by network topology.

Default configuration parameters
--------------------------------

Below are the default configuration parameters for the ``Hierarnet`` model.

.. literalinclude:: ../../src/models/Hierarnet/Hierarnet_cfg.yml
   :language: yaml
   :start-after: ---


.. _literature-ref:
Literature
----------

* Herdeanu, Benjamin (2018). Master's thesis: *Emergence of Cooperation in Evolutionary Social Interaction Networks*. Ruprecht-Karls Universität Heidelberg.
