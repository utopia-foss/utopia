Versioning
==========
Utopia deliberately avoids versioning: You will neither find exact version numbers nor periodic releases of Utopia.
Instead, the idea is to always use the latest version of the framework.

Despite the above approach towards versioning, large infrastructure changes are indicated by an increase in a major version (``vX``) and a corresponding `release branch <https://gitlab.com/utopia-project/utopia/-/branches?state=all&search=v>`_ of the same name.
We do not specify minor version numbers or patch numbers because we do not want to suggest that there is a versioning scheme; we'd like you to *"live at HEAD"*.

So far, there has only been one large enough infrastructure change to warrant a new major version.
The version of Utopia *prior* to that change is labelled ``v1`` – more information on that :ref:`below <utopia_v1>`.
The *current* version of Utopia is referred to as *"latest"*.
The label ``v2`` is avoided to not over-emphasize version numbers.

.. note::

    While there are no version numbers, the Utopia frontend (starting *after* :ref:`utopia_v1`!) keeps track of the state of the involved repositories at the time of a simulation run, see :ref:`here <mv_cfg_backup_repo_info>`.
    This assists in reconstructing the state of a project at the time a model was run.


.. _utopia_v1:

Utopia ``v1``
^^^^^^^^^^^^^
Utopia ``v1`` refers to the state of Utopia prior to the `outsourcing of the utopya frontend package <https://gitlab.com/utopia-project/utopia/-/merge_requests/277>`_ into its `own repository <https://gitlab.com/utopia-project/utopya>`_.

To use Utopia ``v1``, make sure to locally check out the release branch of the same name and stay on that branch.

While we aim to cherry-pick important bug fixes into the branch, future development of Utopia will occur on the main branch, so it's worth considering to upgrade to the latest version of Utopia.

.. _upgrade_from_v1:

Upgrading from ``v1``
^^^^^^^^^^^^^^^^^^^^^
.. NOTE If changing something here, also check whether the information in
        cmake/modules/UtopiaUtopyaCalls.cmake is still valid.
        If not, also update it there.

To upgrade Utopia to its latest version, simply pull the latest changes:

.. code-block:: bash

    # Pull the latest changes
    cd utopia
    git pull

    # Re-configure
    cd build
    cmake ..

In case configuration fails, delete and re-create the ``build`` directory and have a look at the troubleshooting section of the :doc:`README <../README>`.
That's all you need to do to upgrade the Utopia *framework*.

If you are using a :ref:`separate project for model implementations <set_up_models_repo>`, a few changes are necessary to make your project work with the latest version of Utopia again:

- First, upgrade the Utopia *framework* as described above.
- Then, in your models repository, run ``cmake ..``, which should give you an error message coming from the ``register_models_with_frontend`` function.
  That message contains instructions about upgrading your project, in brief:

  - Rename the failing CMake function call to ``register_models_with_utopya``.
  - Create the project info file ``.utopya-project.yml`` in the root directory of your project and add the following content, adjusting the marked entries:

    .. code-block:: yaml

        project_name: YourProjectName              # TODO Set to your project's name
        framework_name: Utopia

        # Information about your project's directory structure
        paths:
          models_dir: rel/path/to/models/src/dir   # TODO Set rel. path to your models
          py_tests_dir: python/model_tests         # if they exist
          py_plots_dir: python/model_plots         # if they exist

        # Metadata about your project (aggregated in utopya models registry)
        metadata:                                  # TODO Adjust to your needs
          authors:
            - your name
            - another author's name
          # Other fields: description, long_name, long_description, license, language, website

    In the ``metadata`` node, you can put in some descriptive info about your models, e.g. a list of `authors` or a short `description`.
    *Hint:* You can also have a look at Utopia's ``.utopya-project.yml`` file to see more possible entries.

  - Delete the ``build/CMakeCache.txt`` file in your models repository to avoid carrying over outdated variables.
    If you get an error in the next step, try deleting the whole ``build`` directory and creating it anew.

  - Now, run ``cmake ..`` again.

- Furthermore, you *may* need to update ``import`` statements in your Python model tests or model plots because the utopya and dantro package structures changed.
  To find out the new import locations, refer to the API references in the `utopya documentation <https://utopya.readthedocs.io/>`_ and `dantro documentation <https://dantro.readthedocs.io/>`_.

If you encounter difficulties in upgrading, we are happy to help; please open an issue in our `GitLab project <https://gitlab.com/utopia-project/utopia/-/issues>`_.



Usage differences
"""""""""""""""""
After upgrading, most things should remain the same compared to ``v1``.

However, there are some differences in the CLI, most notably:

- The ``--no-plot`` options has been renamed to ``--no-eval``.
- The ``--sweep`` and ``--single`` flags have been replaced by the ``--run-mode {sweep,single}`` option.
- The ``--plot-only`` option now only takes a single argument but can be given multiple times, e.g. ``--po some_plot --po some_other_plot``.

For details on the new CLI, have a look at the corresponding ``--help`` in case you encounter errors.
