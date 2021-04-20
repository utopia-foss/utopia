.. _batch:

Batch Framework
===============
Most of the Utopia frontend focusses on making a single simulation or evaluation run as easy and configurable as possible, because working on the level of the individual simulation is the most frequent use case. When working with *multiple* simulations, Utopia can be of help as well:
the Batch Framework allows defining and performing multiple tasks all from the comfort of a single so-called "batch configuration file", or **batch file**.
This batch file can be used to define so-called ``eval`` and ``run`` tasks, corresponding to evaluation and running of simulations, respectively.

.. contents::
    :local:
    :depth: 2

.. note::

    ``run`` tasks are not implemented yet!


.. _batch_cfg:

Batch Configuration
-------------------
Example
^^^^^^^
A batch file may look like this:

.. literalinclude:: ../../../python/utopya/test/cfg/batch.yml
   :language: yaml
   :start-after: # START --- example_basic
   :end-before: # END ----- example_basic


In the above example, two evaluation tasks are defined, ``densities`` and ``spatial``, each loading data from some ``run_dir`` within the Utopia output directory and performing a subset of plots on that data.
To perform these tasks, simply invoke the Utopia CLI:

.. code-block::

    utopia batch path/to/batch_file.yml

You will see some log output that is similar to that of calling ``utopia run``, indicating how far batch processing has proceeded.
Each of the tasks will run in its own process, thereby naively parallelizing the batch tasks; see :ref:`batch_parallel` for more information.

The output is then stored in the so-called **batch output directory**.
By default, this is a directory within ``~/utopia_output/_batch`` that contains the timestamp of the current batch call.
In that directory, backups of all involved configuration files, log outputs, and other meta-data for the batch tasks are stored.

While ``run`` task results will end up in the *regular* Utopia output directory under the specified model name, ``eval`` task results end up inside the *batch* directory, in a separate subdirectory for each task.
For a more detailed overview and configuration options, see :ref:`batch_output`.


Update Scheme
^^^^^^^^^^^^^
The batch configuration consists of four layers:

    1. :py:mod:`utopya` default values, see :ref:`utopya_default_batch_cfg` (the "base configuration")
    2. The :ref:`user-specific defaults <batch_cfg_user>`
    3. The batch file, specified via ``batch_cfg_path`` in the CLI
    4. Runtime update values, e.g. the ``debug`` CLI option

As with the :py:class:`~utopya.multiverse.Multiverse` meta-configuration, these dict-like configuration trees are updated recursively, starting from the first level.
The resulting batch configuration and the involved files are backed up to ``{batch_out_dir}/{timestamp}/config``.

.. hint::

    If you have the feeling that some configuration key is not taken into account, inspecting this file may help you figure out if it ended up in the wrong place without raising an error.

.. _batch_cfg_user:

User-specific batch framework defaults
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The user-specific batch configuration is akin to the :ref:`user configuration <config_hierarchy>` of the :py:class:`~utopya.multiverse.Multiverse` meta-configuration, where often-used default values (e.g. for the ``worker_manager``) can be set. To set values, either directly edit the ``~/.config/utopia/batch.yml`` file or use the CLI:

.. code-block::

    utopia config batch --set worker_manager.num_workers=-1 --get

.. warning::

    This file should not be used to define ``tasks``, as these would be carried out *every time*.


.. _batch_output:

Batch Output
------------
This section details the output directory structure and shows how to configure it to suit your needs. First, let's introduce some terminology:

* **Batch directory** or **batch output directory** refers to the directory where batch task meta-data and output are stored.

    * By default, this is ``~/utopia_output/_batch``
    * It can be configured by the ``paths.out_dir`` option

* The **batch run directory** is the timestamped directory in the batch directory that is created when calling ``utopia batch``.

    * It has the format ``YYMMDD-HHMMSS``, potentially with the ``paths.note`` as a suffix, i.e. ``YYMMDD-HHMMSS_{note}``.
    * It serves as a backup of the used configuration files and stores log output.
    * This is not to be confused with the output of a run *task* (which denotes a simulation run invoked via a batch task).

* The **evaluation output directory** is where output of evaluation tasks is written to.

    * By default, this is the ``eval`` subdirectory within the batch run directory.
    * It can be configured separately for each task, see :ref:`batch_output_custom_eval_outdir`.


The *default* **folder structure** will thus look something like this:

.. code-block:: console

  ~
  ├─┬ utopia_output
    ├── ...                              # other Utopia output
    └─┬ _batch                           # The "batch output directory"
      ├─┬ 201221-094542                  # One "batch run directory"
        ├─┬ config                       # Backup of the batch configuration
          ├── batch_cfg.yml
          ├── batch_file.yml
          ├── update_cfg.yml
          └─┬ tasks                      # Backup of each task configuration
            ├── eval_{task_name}.yml
            └── ...
        ├─┬ eval                         # (Default) output of evaluation tasks
          ├─┬ {task_name}                # Output from one specific task
            └── ...
          └── ...
        ├─┬ logs                         # Logging output from each task
          ├── eval_{task_name}.log
          └── ...
        └── _report.txt                  # The batch report file
      └── ...

Some general remarks:

* The batch directory will *always* be created when ``utopia batch`` is invoked, as it stores the meta-data and log files.
* Output of *run* tasks will be stored to the regular ``~/utopia_output`` directory (or whichever directory you configured as default).
* If the output of *evaluation* tasks is :ref:`stored in a custom directory <batch_output_custom_eval_outdir>`, the ``eval`` subdirectory within the batch run directory will still be created but remain empty.

.. note::

    When running batch evaluation tasks, the **output will not be stored alongside the simulation data** (as it would be if generated via the ``utopia eval`` command) but within the batch run directory or in the custom evaluation output directory.

    The reasoning behind this is that otherwise a batch evaluation would lead to files being created in several places at once, which may be confusing.


Custom batch directory
^^^^^^^^^^^^^^^^^^^^^^
If you changed the default Utopia output directory (``~/utopia_output``), you might also want to change the location of the batch output directory. This can be done conveniently via the :ref:`user-specific configuration <batch_cfg_user>`, or the ``utopia config`` CLI command:

.. code-block::

    utopia config batch --set paths.out_dir=~/my/custom/batch_out_dir --get


.. _batch_output_custom_eval_outdir:

Custom evaluation output directory
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For evaluation tasks, it might be desirable to put the output into a custom directory (e.g. ``my_thesis/figures``) rather than into the default batch output directory. To do so, add the following key to the ``task_defaults.eval`` entry:

.. code-block:: YAML

    task_defaults:
      eval:
        out_dir: "~/path/to/my_thesis/figures/{task_name:}"

This will put all task output into that directory, sorted under their names.

Other available keys for that format string are ``model_name`` and ``timestamp``, where ``timestamp`` refers to the time of the ``utopia batch`` invocation.

.. note::

    Be aware that writing to the same directory may lead to ``FileExistsError``\ s for the plot configurations that are saved alongside each plot or for the creation of the output directory.
    You will need to adjust the ``DataManager`` and ``PlotManager`` configurations accordingly:

    .. code-block:: YAML

        task_defaults:
          eval:
            data_manager:
              out_dir_kwargs:
                # Allow that the data directory may already exist
                exist_ok: true

            plot_manager:
              # Allow config files to be overwritten
              cfg_exists_action: overwrite_nowarn

              # For each creator, allow that individual plot files may
              # already exist, thus *not prohibiting* overwriting
              creator_init_kwargs:
                  universe:
                    exist_ok: true
                  multiverse:
                    exist_ok: true

    You can also set ``plot_manager.save_plot_cfg: false`` to disable writing plot configuration files altogether.


Crosslinking
^^^^^^^^^^^^
The batch framework can create several symlinks to improve navigation between directories related to a certain task.
For example, this can help with retrieving the configuration options that were used to create the output, e.g. when using a custom output directory.

To control this option, use the ``create_symlinks`` option in each task configuration.

The following symlinks are created:

* From the evaluation output directory to the batch *task* configuration
* From the evaluation output directory to the *run* directory of the selected simulation
* From the ``eval`` directory in the run directory of the selected *simulation* to the custom evaluation output directory
* If a custom evaluation output directory is used:

    * A link is added to the *default* evaluation output directory within the batch run directory, pointing to the custom one.
    * A link is added from the custom output directory to the batch run directory.


.. note::

    If using synchronization tools for file syncing between hosts, the above procedure may be problematic.
    In case the synchronization tool is not able to properly handle symlink loops, either configure it to not follow symlinks or try disabling this option:

    .. code-block:: YAML

        task_defaults:
          eval:
            create_symlinks: false



.. _batch_parallel:

Parallelization
---------------
Batch tasks are being worked on in parallel using the :py:class:`~utopya.workermanager.WorkerManager`.
Each task creates its *own process*, loading the ``utopya`` module and working on the given instructions separately from every other task.

The **parallelization level** may be controlled by the ``parallelization_level`` configuration option:

* ``batch`` parallelization (the default) means that the *batch* tasks are being worked on in parallel while the individual tasks are meant to use only a single CPU core.
* ``task`` parallelization means that the batch tasks are worked on sequentially, leaving parallelization to the *individual* tasks.

Under the hood, each task is worked in a separate ``multiprocessing.Process``, which is handled by :py:class:`~utopya.task.MPProcessTask`.
Independent of the platform, processes are created with the ``spawn`` method, thus making them fully independent from the parent process.
There is no option to share memory between processes; this would be too difficult and the speedup would most probably evaporate.

.. hint::

    Keep a lookout for memory usage of ``utopia batch``:
    running multiple memory-hungry evaluation tasks in parallel can lead to trouble!
    In such a case, consider setting a different ``parallelization_level``.



.. _batch_remarks:

Remarks
-------
* To **disable** individual tasks, add the ``enabled: false`` key.
* Tasks can be associated with a ``priority``, where a lower value means that the tasks will be worked on first.
* Use the ``debug`` option (on the *root level* of the batch configuration) to let a failing task lead to the stopping of all other tasks.
  Otherwise, a failing batch task will only lead to a warning in the output log.
* Typically, **stream forwarding** does not make sense with multiple tasks running at the same time because the log would become garbled.
  To still enable it in ``batch`` parallelization mode, set ``worker_kwargs.forward_streams: true``.
