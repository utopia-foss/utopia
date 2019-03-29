
``mesonet`` — Model of Consumer-Resource Systems
================================================

``mesonet`` models the interaction of consumer species with the resources in their environment.

This implementation is preceded by a `Python implementation <https://ts-gitlab.iup.uni-heidelberg.de/yunus/mesonet>`_ of the same name, where the interactions were represented using a network.
The present Utopia implementation foregoes the network approach; while this restricts interactions to the consumer-resource scenario, the implementation is simpler, faster, and allows to explore other aspects of the model.

*This document is WIP*


Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``mesonet`` model.

.. literalinclude:: ../../src/models/mesonet/mesonet_cfg.yml
   :language: yaml
   :start-after: ---
