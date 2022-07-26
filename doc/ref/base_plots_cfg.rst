.. _utopia_base_plots_ref:

Base Plot Configuration Pool
============================
This page documents Utopia's base plot configuration pool which is used for aggregating a plot configuration using `multiple inheritance <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html#plot-configuration-inheritance>`_.

.. note::

    **Important:** All entries here are **in addition** to the :ref:`dantro base plots configuration pool <dantro_base_plots_ref>` *and* the :ref:`utopya base plots configuration pool <utopya_base_plots_ref>`.

    More precisely, the Utopia base plots config pool *requires* the above pools, as it :ref:`inherits <plot_cfg_inheritance>` some entries using the ``based_on`` key in a plot configuration.

    The lookup order is as follows:

    1. :ref:`dantro base plots <dantro_base_plots_ref>`
    2. :ref:`utopya base plots <utopya_base_plots_ref>`
    3. Utopia base plots *(this file)*
    4. *If in a separate model projects:* Project-specific base plots
    5. Model-specific base plots

    Note that you can also :ref:`adjust base plot configurations <custom_base_config_pools>`.

The entries below are sorted by segments and are using dantro's `naming convention <https://dantro.readthedocs.io/en/latest/plotting/plot_manager.html#base-plots-naming-conventions>`_.
Some sections are still empty, meaning that Utopia does not define new entries there.

.. hint::

    To quickly search for individual entries, the search functionality of your browser (``Cmd + F``) may be very helpful.
    Note that some entries (like those of the YAML anchors) may only be found if the :ref:`complete file reference <utopya_base_plots_ref_complete>` is expanded.

.. contents::
    :local:
    :depth: 2

----


``.defaults``: default entries
------------------------------

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .defaults
    :end-before: # ===



``.creator``: selecting a plot creator
--------------------------------------
More information: :ref:`plot_creators`

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .creator
    :end-before: # ===


``.plot``: selecting a plot function
------------------------------------
More information:

- :ref:`plot_func`
- :ref:`pcr_pyplot_plot_funcs`

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .plot
    :end-before: # ===


``.skip``: skip conditions
--------------------------
These can be used to skip ``multiverse`` plots if a sweep did not have a certain dimensionality.
This can be useful to restrict the scope of automatically created :ref:`facet grid plots <facet_grid_auto_encoding>`.

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .skip
    :end-before: # ===


``.style``: choosing plot style
-------------------------------
More information:

- :ref:`pcr_pyplot_style`
- `Matplotlib style sheets reference <https://matplotlib.org/gallery/style_sheets/style_sheets_reference.html>`_
- `Matplotlib RC parameters <https://matplotlib.org/stable/tutorials/introductory/customizing.html>`_

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .style
    :end-before: # ===


``.hlpr``: invoking individual plot helper functions
----------------------------------------------------
More information: :ref:`plot_helper`

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .hlpr
    :end-before: # ===


``.animation``: controlling animation
-------------------------------------
More information:

- :ref:`dantro docs on animations <pcr_pyplot_animations>`
- :ref:`Examples <plot_animations>`

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .animation
    :end-before: # ===


``.dag``: configuring the DAG framework
---------------------------------------
More information:

- :ref:`dag_framework`
- :ref:`plot_creator_dag`

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .dag
    :end-before: # ===


``.dag.meta_ops``: meta operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following entries can be included into a plot configuration to make pre-defined :ref:`meta-operations <dag_meta_ops>` available for the data transformation framework.

.. literalinclude:: ../../python/utopia_base_plots.yml
    :language: yaml
    :start-after: # start: .dag.meta_ops
    :end-before: # ===


----

.. _utopia_base_plots_ref_complete:

Complete File Reference
-----------------------

.. toggle::

    .. literalinclude:: ../../python/utopia_base_plots.yml
        :language: yaml
        :end-before: # end of Utopia base plots
