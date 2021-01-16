
.. _faq_config:

Configuring Simulations
-----------------------
For a general overview regarding configuration, see :ref:`run_config`.
For reading configuration keys in the model, see the :ref:`model configuration FAQs <faq_model_config>`.

How can I find the available model parameters?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The base configuration documents a lot of parameters directly in the configuration file; see :ref:`here <utopya_base_cfg>`. For the model configuration, the model documentation usually includes the default configuration; for example, take a look at the :doc:`../../models/ForestFire`.


What's with all these YAML ``!tags``? What can I use them for?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
YAML tags are really cool!
You've probably already discovered the ``!sweep`` tag, which is used to define a :ref:`parameter sweep dimension <run_parameter_sweeps>`. Another example is ``!param``, used to mark parameters that :ref:`need to be validated <config_validation>` before a simulation run. When reading YAML files, the frontend can attach certain functionality to entries that are labelled with a ``!tag``. Throughout Utopia (and its dependencies, ``dantro`` and ``paramspace``), this functionality is used to make it more convenient to define configuration entries.

Which other tags are available?
"""""""""""""""""""""""""""""""
Quite a bunch. The example below demonstrates them:

.. literalinclude:: ../../../python/utopya/test/cfg/doc_examples/faq_frontend.yml
    :language: yaml
    :start-after: ### Start -- faq_frontend_yaml_tags_general
    :end-before:  ### End ---- faq_frontend_yaml_tags_general
    :dedent: 2

There are some **Python-only tags**, which create Python objects that have no equivalent on C++-side.
Make sure to *not* specify them inside your run or model config, but *only* in your evaluation config.

.. literalinclude:: ../../../python/utopya/test/cfg/doc_examples/faq_frontend.yml
    :language: yaml
    :start-after: ### Start -- faq_frontend_yaml_tags_python_only
    :end-before:  ### End ---- faq_frontend_yaml_tags_python_only
    :dedent: 2

.. hint::

    Make sure to check out the `paramspace docs <https://paramspace.readthedocs.io/en/latest/yaml/supported_tags.html>`_ for more YAML tags, which e.g. allow evaluating simple boolean operations or format strings.
