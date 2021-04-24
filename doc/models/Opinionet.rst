.. _model_Opinionet:

``Opinionet`` – Opinion Dynamics
================================

``Opinionet`` is a model of opinion dynamics on a social network. It simulates a network of users (nodes in a network), who update their opinions upon interaction with their social contacts (their neighboring nodes).
This model includes a variety of different opinion dynamics models from the literature, including the Hegselmann-Krause (2002) and Deffuant (2000) models. The one-dimensional opinion space can be chosen to be discrete or continuous, and network edge weights can be configured to represent interaction probabilities.
The ``Opinionet`` model is designed for further expansion and adaptation by researchers of social network opinion dynamics.

.. contents::
   :local:
   :depth: 2

----

Implementation
--------------

Opinion space
^^^^^^^^^^^^^
The network consists of nodes (representing users) and edges (representing social links). Each user holds an *opinion*, a value in the one-dimensional *opinion space*. In the model, the opinion space can be either *continuous* or *discrete*. A continuous opinion space represents views held on a spectrum with two opposing extremes (e.g. ranging from 'yes' to 'no', 'agree' to 'disagree', etc.), and is modeled as a real interval :math:`[a, b] \subset \mathbb{R}`; a discrete space holds views on a topic with a countable number of distinct choices (e.g. either 'yes' or 'no', different preferences for political parties, etc.), and is modeled as a series of integers :math:`\{0, ..., N\}`.

.. hint:: You can control the opinion space via the ``opinion_space`` key (see below).

.. _interaction_functions:

Interaction functions
^^^^^^^^^^^^^^^^^^^^^

There are two basic types of interaction functions: the *Deffuant* type, and the *Hegselmann-Krause* type. In addition, two parameters are relevant to the opinion interaction: the *tolerance* :math:`\epsilon`  and the *susceptibility* :math:`\mu`. The tolerance determines which spectrum of opinions a user is willing to engage with. A small tolerance bound means users only interact with opinions very close to their own; a large tolerance means users are rather more 'broad-minded'. The susceptibility, in general, models how susceptible users are to others' opinions, that is, to what degree they are willing to shift their own opinion towards that of others.

**Hegselmann-Krause (HK) model:** In the HK model, users shift their opinions towards the average opinion of *all of their neighbors*. In a single step, a randomly chosen vertex updates its opinion via

*(a) Continuous opinion space:*

.. math::

    \sigma_i(t+1) = \sigma_i(t) + \mu * (\langle\sigma_k \rangle - \sigma_i(t) ),
    
*(b) Discrete opinion space:*

.. math::

    \sigma_i(t+1) = [\sigma_i(t) + \mu * (\langle\sigma_k \rangle - \sigma_i(t) )],

where :math:`[x]` denotes rounding to the nearest integer.

Here, :math:`\langle \sigma_k \rangle` is the average opinion of all neighboring vertices whose opinions fall within the tolerance range, i.e. for whom :math:`\vert \sigma_i - \sigma_k \vert <  \epsilon`.

In a directed network, this average is weighted, meaning the neighbors' opinions are weighted using the *edge weights* (see :ref:`network`) normalised to the number of actually interacting neighbors.


**Deffuant model:** In the Deffuant model, only *pairs of users* interact. A random vertex :math:`i` and one of its neighbors :math:`k` are chosen, and the vertex updates its opinion via

*(a) Continuous opinion space:*

.. math::

    \sigma_i(t+1) = \sigma_i(t) + \mu * (\sigma_k - \sigma_i(t)),

*(b) Discrete opinion space:*

.. math::

    \sigma_i(t+1) = \sigma_k

with probability :math:`\mu`.

How the neighbor is chosen depends on the network: in a directed network, the probability is equal to the edge weights (see below). In an undirected network, a neighbor is chosen with uniform probability.

.. note:: With these implementations, user opinions will always remain in the opinion space, whether discrete or continuous. For :math:`\mu = 1`, the neighbors' opinion is always adopted entirely.

.. hint:: You can control the interaction function, tolerance, and susceptibility through the keys of the same name. Note that the tolerance can be arbitrarily large, though it loses interpretability when negative. The susceptibility must be in :math:`[0, 1]`.

.. _network:

Network
^^^^^^^

In each iteration of the model, a single user is chosen at random with uniform probability, and this user's opinion is updated through its interaction with other users. Which users interact is determined through the network topology. Utopia allows choosing :ref:`different types of underlying topologies<graph_gen_functions>` for the network (e.g. small-world or scale-free), or :ref:`loading your own network from a dataset<loading_a_graph_from_a_file>`. The network can be directed or undirected, and edges can be given *weights*, representing the probability of the end nodes interacting.

.. hint:: You can control the network topology through the ``network/model`` key, and the directedness through the ``directed`` key.

**Undirected network:** In an undirected network, links have no orientation, and (in this model) hold no edge weights. In the Deffuant model of opinion dynamics, only pairs of nodes interact in one time step (see :ref:`interaction_functions`). In an undirected network, all neighbors of a given node have an equal probability of being selected as an interaction partner.

**Directed network:** In a directed network, edges have an orientation. Imagine a network of Twitter users: person A may follow person B, but person B does not necessarily follow person A back. Vertices therefore have an *out-degree* (people they follow), and an *in-degree* (people they are followed by). In the directed network, each link is given an *edge weight* :math:`w \in [0, 1]`. This weight plays a role in selecting neighbors for interaction (in the Deffuant model) or in giving weight to neighbors' opinions (in the Hegselmann-Krause model) – more on that below.

**Edge weights:** The edge weights are calculated using a softmax function. Let :math:`\Delta \sigma_{i,j}` be the opinion difference :math:`\vert \sigma_i - \sigma_j \vert` between users :math:`i` and :math:`j`. The weight on edge :math:`i, j` is then set to

.. math::
    
    w_{i, j} = \dfrac{e^{-w \Delta \sigma_{i, j}}}{\sum_{k} e^{-w \Delta \sigma_{i, k}}},
    
where the sum over :math:`k` ranges over all neighbors of :math:`i` (softmax function). The parameter :math:`w>0` is the *weighting parameter*. It controls how sharply the edge weights decrease with the opinion difference. For :math:`w=0`, the edge weights are all equal to 1/out degree(i).

.. hint:: You can control the weighting parameter via the ``network/edges/weighting`` key. It only has an effect when the network is directed.

.. warning:: Extremely large ``weighting`` parameters (:math:`w \gg 10`) can lead to memory underflow, and weights will be written as zero.

.. note:: When the network is directed, edge weights are saved to an ``edge_weights`` dataset. You can use the edge weights in the graph plotting function, for instance to define the width of the links (see :ref:`plotting`).

**Rewiring:** The topology of the network does not have to be static. You can let users cut links and rewire to new neighbors via the ``rewiring`` key. If activated, a randomly selected link between users whose opinions are further apart than the tolerance :math:`\epsilon` is rewired to a new, randomly chosen neighbor.



How to run the model
--------------------

Model parameters
^^^^^^^^^^^^^^^^

.. raw:: html

   <details>
   <summary><a>See the model config</a></summary>

.. literalinclude: .. raw:: html

   <details>
   <summary><a>big code</a></summary>

.. literalinclude:: ../../src/utopia/models/Opinionet/Opinionet_cfg.yml
   :language: yaml
   :start-after: ---

.. raw:: html

   </details>
   
The following keys in the model configuration allow you to control the model:

- ``opinion_space``:

    - ``type``: whether the opinion space is ``continuous`` or ``discrete``.
    
    - ``interval``: if the opinion space is continuous: a real, closed interval.
    
    - ``num_opinions``: if the opinion space is discrete: the number of discrete opinions.

- ``tolerance``: a real, positive value representing the confidence bound.

- ``susceptibility``: a real, positive value in :math:`[0, 1]`.

- ``network``:

    - ``directed``: whether or not the network should be directed. If directed, the network edges will be given weights (see above).
    
    - ``model``: the network topology: can be ``ErdosRenyi`` (random), ``WattsStrogatz`` (small-world), ``BarabasiAlbert`` (scale-free undirected), ``BollobasRiordan`` (scale-free directed), or ``load_from_file`` (see :ref:`here<graph_gen_functions>`).
    
    - ``edges``:
    
        - ``weighting``: the weighting parameter used in the softmax function to set the edge weights (see above). Must be a postive real.
        
        - ``rewiring``: whether or not edges between users with large opinion differences are rewired.


   
 

.. _plotting:

Plotting
^^^^^^^^

``Opinionet`` comes with several default plots:

**Graph plots:**

- ``graph``: plots a single snapshot of the network at time ``time_idx``. Node and edge properties can be plotted; by default, the node size is the (out-)degree, the node color its opinion. For directed graphs, the edge widths can be configured to match the edge weights.

- ``graph_animation``: animated plot of the network over time.

**Universe plots:**

- ``opinion_animated``: an animated histogram of the opinion distribution over time. The plot can also output the distribution at a single timeframe using the ``time_idx`` key.

- ``opinion_time_series``: plots the temporal development of the opinion density, as well as the final opinion distribution, as well as some representative trajectories of users in the largest opinion groups in the final distribution. This can be controlled from the configuration, e.g. by specifying the number of representatives (``max_reps``).

**Multiverse plots:**

Various data analytical parameters can be plotted for multiverse runs, e.g. as a 1d errorbar, or a 2d heatmap. These plots use the DAG framework. Current parameter options are:

- ``op_number_of_peaks``: calculate the number of opinion peaks

- ``op_localization``: yields a measure of how condensed the opinion distribution is

- ``op_polarization``: the polarization (in analogy with the physical definition) of the opinion distribution

In each case, the time of the opinion distribution in question can be specified.

.. raw:: html

   <details>
   <summary><a>See the default plot configuration</a></summary>

.. literalinclude: .. raw:: html

   <details>
   <summary><a>big code</a></summary>

.. literalinclude:: ../../src/utopia/models/Opinionet/Opinionet_plots.yml
   :language: yaml
   :start-after: ---

.. raw:: html

   </details>

.. raw:: html

   <details>
   <summary><a>See the base plot configuration</a></summary>

.. literalinclude: .. raw:: html

   <details>
   <summary><a>big code</a></summary>

.. literalinclude:: ../../src/utopia/models/Opinionet/Opinionet_base_plots.yml
   :language: yaml
   :start-after: ---

.. raw:: html

   </details>

For the utopya base plots, see :ref:`utopya_base_cfg`.


Literature
----------
- Deffuant G. et al: *Mixing beliefs among interating agents.* Adv Complex Syst. (2000) **3**:87-98.
- Hegselmann, R. & Krause, U. (2002). *Opinion Dynamics and Bounded Confidence Models, Analysis and Simulation.* J. Artificial Societies and Soc. Simulation **5**  3: 1–33.
- Gaskin, Thomas (2020). Master's thesis: *Modelling Homophily and Discrimination in Selective Exposure Opinion Dynamics.* Heidelberg University. Download `here  <https://1drv.ms/b/s!AnHsRv-O4KyKg_B2pPTiCFEBrFthww?e=LCb2jj>`_.
- Traub, Jeremias (2019). Bachelor’s thesis: *Modelling Opinion Dynamics – Selective Exposure in Adaptive Social Networks.* Heidelberg University.

**Further reading:**

- Arendt, D. L. & Blaha, L. M. (2015). *Opinions, influence, and zealotry: a computational study on stubbornness.* Comp. Math. Organization Theory **21** 2: 184–209.
- Axelrod, R. (1997). *The dissemination of culture: a model with local convergence and global polarization.* J. Conflict Resolution 41: 203–226.
- Baumann, F. et al. (2020b). *Emergence of polarized ideological opinions in multidimensional topic spaces.* arXiv:2007.00601 [physics.soc-ph].
- Carro, A. et al. (2013). *The Role of Noise and Initial Conditions in the Asymptotic Solution of a Bounded Confidence, Continuous-Opinion Model.* J. Stat. Phys. **151**: 131–149.
- Castellano, C. et al. (2009). *Statistical physics of social dynamics*. Reviews of Modern Physics **81** 2: 591–646.
- Del Vicario, M. et al. (2016). *The spreading of misinformation online.* Proc. Nat. Acad. Sc. USA. **113** 3: 554–559.
- Flache, A. et al. (2017). *Models of Social Influence: Towards the Next Frontiers.* J. Artifical Societies and Soc. Simulation **20**  4.
- Guerra, P. H. C. et al. (2013). *A measure of polarization on social media networks based on community boundaries.* Proc. Int. AAAI Conf. Web and Social Media (ICWSM’13).
- Kozma, B. & Barrat, A. (2008). *Consensus formation on adaptive networks.* Phys. Rev. E **77**, 016102.
- Mäs, M., Flache, A., & Helbing, D. (2010) *Individualization as Driving Force of Clustering Phenomena in Humans.* PLoS Comput. Biol. **6** 10: e1000959.
- Perra, N. & Rocha, L. E. C. (2019). *Modelling opinion dynamics in the age of algorithmic personalisation.* Sci. Rep. **9** 7261.
- Sobkowicz, P. (2012). *Discrete Model of Opinion Changes Using Knowledge and Emotions as Control Variables.* PLoS ONE **7** 9: e44489.
- Sznajd-Weron, K. (2005). *Sznajd model and its applications.* Acta Physica Polonica B **36** 8: 2537–2547.

