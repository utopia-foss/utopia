.. _model_CopyMeGraph:

``CopyMeGraph`` — A good place to start with your graph-based model
===================================================================

This template model supplies a minimal graph-based Utopia model.
On top of what the :ref:`CopyMeBare model template provides <model_CopyMeBare>`, this model contains:

* Infrastructure for graph setup and data writing
* A customisable ``GraphType`` (based on the `Boost Graph Library <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/index.html>`_), including vertex and edge properties
* Configuration files showcasing some of the graph setup and plotting capabilities
* A ``cfgs`` directory with further example configuration files

Use this model if you want to start "from scratch" but want to avoid setting up the infrastructure itself.

.. hint::

    Refer to :ref:`impl_step_by_step` for information on how to implement your own model based on this template.



Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``CopyMeGraph`` model.

.. literalinclude:: ../../src/utopia/models/CopyMeGraph/CopyMeGraph_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``CopyMeGraph`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/CopyMeGraph/CopyMeGraph_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/CopyMeGraph/CopyMeGraph_base_plots.yml
   :language: yaml
   :start-after: ---

For available base plots, see :ref:`utopia_base_plots_ref`.
