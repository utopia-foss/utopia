.. _config_validation:

Validating model parameters
===========================

When running your model, you always want to be sure only valid parameters have been passed, since for invalid parameters the model will either break or show unexpected behaviour.
For instance, certain parameters may need to be integers or probabilities, or you may want string parameters to be one of only a few permissible options.

Utopia allows to specify validity constraints on your model's parameters and have them automatically checked *before* a simulation run and without the need to include those checks into your model implementation.

You can define a parameter's default value, its data type, range limits, or pass a set of accepted values.
In case of any errors, a concise message is logged and the run terminated.
Additionally, you can directly add a clear name and descriptive text to your parameter, making that information parseable and reducing the need for comment strings.

For certain frequently used parameter types (such as probabilities), a convenient :ref:`shorthand notation <config_validation_shorthands>` is available.

.. contents::
    :local:
    :depth: 2

.. note::

    This feature currently only supports validating *scalar* nodes.
    You cannot validate sequences or the structure of mappings.

----

Basic syntax
------------
Validation parameters can be optionally specified on all *scalar* parameters in your default model configuration file, i.e. inside the ``<your_model_name>_cfg.yml`` file.
The rationale behind specifying it in that file is to have all information on model parameters in one place.

If you wish to have a model parameter validated, simply add a ``!param`` tag after the parameter name, and specify the default value and any validity constraints in an *indented block* underneath.
A ``default`` value is required, though the validity restrictions are optional.

Refer to the documentation below and to the examples to learn more about possible keys.

.. automethod:: utopya.parameter.Parameter.__init__
   :noindex:

.. warning::

    ``!param`` can **only** be specified inside your default model configuration, e.g. ``MyFancyModel_cfg.yml``.
    Specifying it elsewhere will lead to errors!

Examples
^^^^^^^^

.. literalinclude:: ../../../python/utopya/test/cfg/doc_examples/param_validation.yml
    :language: yaml
    :start-after: ### Start -- config_validation_simple_tags
    :end-before:  ### End ---- config_validation_simple_tags

.. hint::

    Use YAML's ``~`` (i.e., ``None``) for specifying ±inf in ``limits``.


.. _config_validation_shorthands:

Shorthands
----------
For frequently used parameter types, shorthand tags are available to spare you the trouble of having to specify limits or data types.
Simply add the corresponding YAML tag and the default value directly:

.. literalinclude:: ../../../python/utopya/test/cfg/doc_examples/param_validation.yml
    :language: yaml
    :start-after: ### Start -- config_validation_shorthand_tags
    :end-before:  ### End ---- config_validation_shorthand_tags

Available shorthands are:

* ``is-probability`` (a ``float`` in range ``[0, 1]``)
* ``is-int``
* ``is-bool``
* ``is-string``
* ``is-positive``/``is-negative``: strictly greater/less than zero
* ``is-unsigned``: greater or equal to zero and data type ``uint``
* ``is-positive-int``/``is-negative-int``: strictly greater/less than zero and data type ``int``

See also the :ref:`Forest Fire model <FFM_cfg>` for an integrated demo of this feature.


Validation procedure
--------------------

Parameter validation happens as part of the :py:class:`~utopya.multiverse.Multiverse` initialization and before any simulations are started.

The validation is restricted to the ``parameter_space`` entry of the :ref:`meta configuration <feature_meta_config>`, i.e. all values that will be made available to the model executable.
If ``parameter_space`` actually is a parameter space, validation will *always* occur for the full parameter space and including the default point, independent of whether a sweep will actually be performed.

For all specified validation parameters, the given value is checked against the constraints using the :py:class:`~utopya.parameter.Parameter`\ 's :py:meth:`~utopya.parameter.Parameter.validate` method:

.. automethod:: utopya.parameter.Parameter.validate
   :noindex:

.. hint::

    For very large sweeps (parameter space volume > 10k), parameter validation may take a long time and can thus be optionally disabled.

    To **skip parameter validation**, set the following entry on the top-level of the run configuration:

    .. code-block:: yaml

        perform_validation: false

    Alternatively, you can specify ``--skip-validation`` via the command line interface, see ``utopia run --help``.
