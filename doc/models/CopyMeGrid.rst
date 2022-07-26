.. _model_CopyMeGrid:

``CopyMeGrid`` — A good place to start with your CA-based model
===============================================================

This template model supplies a minimal CA-based Utopia model.
On top of what the :ref:`CopyMeBare model template provides <model_CopyMeBare>`, this model contains:

* Infrastructure for ``CellManager`` setup and data writing
* A customisable ``CellState``, including a configuration-based constructor
* The accompanying model configuration and evaluation files
* A ``cfgs`` directory with further example configuration files

Use this model if you want to start "from scratch" but want to avoid setting up the infrastructure itself.

.. hint::

    Refer to :ref:`impl_step_by_step` for information on how to implement your own model based on this template.



Default configuration parameters
--------------------------------
Below are the default configuration parameters for the ``CopyMeGrid`` model.

.. literalinclude:: ../../src/utopia/models/CopyMeGrid/CopyMeGrid_cfg.yml
   :language: yaml
   :start-after: ---


Available plots
---------------
The following plot configurations are available for the ``CopyMeGrid`` model:

Default Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/CopyMeGrid/CopyMeGrid_plots.yml
   :language: yaml
   :start-after: ---

Base Plot Configuration
^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../../src/utopia/models/CopyMeGrid/CopyMeGrid_base_plots.yml
   :language: yaml
   :start-after: ---

For available base plots, see :ref:`utopia_base_plots_ref`.
