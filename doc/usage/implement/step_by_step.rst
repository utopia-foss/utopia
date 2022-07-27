.. _impl_step_by_step:

Step-by-step Guide
==================

After having worked through the :doc:`README <../../README>`, the
:ref:`tutorial <tutorial>`, and (optionally) the :ref:`workflow <dev_workflow>`, this guide is for creating a new model in Utopia.

If you go through all the steps, you will end up with a model that can profit
from all Utopia features, but doesn't really do anything terribly interesting yet.
It is merely a **starting point** for your own expedition into the Utopia world.
You will be the one who afterwards defines the rules, entities etc. of
that world.

To that end, there are three models supplied that can be used as a basis for
new models: the so-called ``CopyMe`` models.
They not only showcase some Utopia functionality, but also provide good
starting conditions for different scenarios:

* ``CopyMeGrid``: a basic Cellular Automaton model.

  * Already includes the ``CellManager``.
  * Recommended if you want to work with a Cellular Automaton.

* ``CopyMeGraph``: a basic graph model.

  * Already includes a graph and example functionality.
  * Recommended if you want to work with a graph.

* ``CopyMeBare``: the bare basics. Really.

  * Recommended if you do not need a cellular automaton or a graph.
  * Recommended if you are proficient with Utopia.

You should now decide with which model you want to start with.
This could be one of the above ``CopyMe`` models, but of course you can also take a pre-implemented model and adapt it to your needs.

⚠️ **Important:** In the following, you will need to replace all mentions of ``CopyMe`` with the name of your own model.


.. admonition:: Model development in docker image

    You **can** use the `Utopia docker image <https://hub.docker.com/r/ccees/utopia>`_ for model development.
    Follow the instructions given there to find out how.



Choosing a name for your model
------------------------------
This is the point where you should decide on the name of your new Utopia model.

For the purpose of this guide, we assume you want to implement a model called ``YourModelName``.
Probably, you will give it a more suitable name.
So, keep in mind to replace every ``YourModelName`` below with the actual model name.

.. note::

    Utopia has a naming convention for models.
    Your model name should consist of words that start with Capital Letters and are ``DirectlyConcatenatedWithoutSeparatingSymbols``.

    Also, you should not include the ``Model`` string into the name, e.g.: you *should* name your model ``ForestFire`` rather than ``ForestFireModel``
    (so ``YourModelName`` is actually not the best example ;) ).


Setting Up The Infrastructure
-----------------------------
Ok, let's get started by setting up the model infrastructure. If you wish to use one of the pre-implemented models as a basis for your new model, Utopia provides a command to do all the copying, renaming of files, and refactoring of file content:

.. code-block:: bash

    (utopia-env) $ utopia models copy ModelToCopy --new-name YourModelName --dry-run

Replace ``YourModelName`` with whatever you wish to call your model, and you're ready to go!
The ``--dry-run`` flag will first show a preview of what would be copied; remove that flag only when you have checked that the effect is as intended.

.. note::

    The command above will prompt for a *target project* to copy the model to.

    * If you want to copy into the Utopia project, specify ``Utopia``.
    * If you want your new model in your own models repository, see :ref:`set_up_models_repo`.
      After having set up that repository, come back here to continue and use the chosen project name in the prompt, e.g. ``UtopiaModelsEvolution``.

The copying tool creates new directories inside ``src/utopia/models`` (or the corresponding directory in your own models repository) and copies model files there, applying a number of replacements.
In addition, the files from ``python/model_plots`` and ``python/model_tests`` are copied following an equivalent procedure.

See ``utopia models copy --help`` for more usage information.

Having done that, you can *skip* the manual copying description below and continue with :ref:`impl_step_by_step_adapt`.


Copying Models Manually
^^^^^^^^^^^^^^^^^^^^^^^
If you want, you can also do these steps manually: go through the following points from top to bottom, and first read the entire instructions for one step before starting to carry it out. Here, ``CopyMe`` refers to the model you wish to copy:

1. Navigate to the ``src/utopia/models`` directory inside the ``utopia`` repository, duplicate the ``CopyMe`` directory of your choice, and rename it to the name of your model (``YourModelName`` above).

2. Rename all the files inside of the newly created directory such that all
   occurrences of ``CopyMe`` are replaced by ``YourModelName``.

  - You can do so by using the `parameter expansion capabilities <http://wiki.bash-hackers.org/syntax/pe>`_ of BASH: inside your model directory, call

  .. code-block:: bash

    for file in CopyMe*; do mv $file ${file/CopyMe/YourModelName}; done

3. Tell Utopia that there is a new model, e.g. include your model in the
   Utopia CMake build routine:

  - In ``src/utopia/models/``, you will find a ``CMakeLists.txt`` file. Open it and let
    CMake find your model directory by including the command:
    ``add_subdirectory(YourModelName)``.
  - In ``src/utopia/models/YourModelName/``, there is another ``CMakeLists.txt`` file.
    Open it and change the line ``add_model(CopyMe CopyMe.cc)`` to
    ``add_model(YourModelName YourModelName.cc)``. With this command, you are telling
    CMake to keep track of a new model.
    

4. In ``YourModelName.cc`` in the ``src/utopia/models/YourModelName/`` directory, replace every ``CopyMe`` with ``YourModelName``. In ``YourModelName.hh``, replace every ``CopyMe`` by ``YourModelName`` and every ``COPYME`` by ``YOURMODELNAME``.

5. Do the same in the  ``YourModelName_plots.yml``, ``YourModelName_base_plots.yml``, and ``YourModelName_cfg.yml`` files.

6. Now check if everything works as desired. For that, enter the ``build`` directory and run ``cmake ..``. Check that the CMake log contains ``Registered model target: YourModelName``. Now execute ``make YourModelName``.

  * Are there errors? Then check that you adjusted everything as
    described above.
  * Building succeeds? Congratulations! 🎉

7. Use the command line interface to run the model:

  .. code-block:: bash

     cd build
     source ./activate
     utopia run YourModelName

If everything works, let's continue with setting up the
testing and plotting framework. You can set up a simple Python testing framework in the following way:

8. Navigate to the ``python/model_tests`` directory, copy the ``CopyMe`` directory and rename it to ``YourModelName``. Make sure that there is a file named ``__init__.py`` inside the directory.
9. Inside the created ``YourModelName`` directory, rename the ``test_CopyMe.py`` file to ``test_YourModelName.py``. Open the ``test_YourModelName.py`` file and replace every ``CopyMe`` with ``YourModelName``.

In this ``test_YourModelName.py`` file you can add tests to your model.
You have the full capabilities of `pytest <https://pytest.org>`_ available plus
the ``utopya.testtools`` module (as exemplified in the ``CopyMe`` model tests.)

.. note::

  Remember to remove the provided example tests if you remove unneeded parts
  of the former ``CopyMe`` model. Otherwise, you will get error messages when
  running the model.


As you saw in the :ref:`tutorial <tutorial>`, it is possible to have custom model plots tailored to the data your model is producing.
You can set them up in the following way:

10. Navigate to the ``python/model_plots`` directory, copy the ``CopyMe`` directory and rename it to ``YourModelName``. Make sure that there is a file named ``__init__.py`` inside the directory.

The ``*_plots.yml`` files you copied alongside the model configuration control
the behavior of the plotting framework. In the ``YourModelName_plots.yml`` file,
you can specify which plots are to be performed automatically.

The ``state.py`` script is provided to show you how a model specific plotting
script could look like.
In ``generic.py`` you see some examples of generic plotting functions which can
be used in combination with Utopia's :ref:`data transformation and selection
framework <plot_with_DAG>`.

When starting to implement more plots, you should definitely have a look at
the :ref:`detailed plotting documentation <eval_plotting>`!

.. note::

    Once you change parts of the former ``CopyMe`` model code, the plots might
    break and you might get errors during plot creation. To alleviate them,
    either adapt the plotting functions, remove them, or temporary disable
    them in the plot configuration (using ``enabled: false``) until you have
    adapted them.



.. _impl_step_by_step_adapt:

Adapting your code
------------------
Depending on what model you want to implement, you will need to delete or
adapt some provided functions.
So, feel free to remove anything you do not
need.

* All variables, functions, etc. that are just there to show how you would use and implement them are denoted with the prefix ``some_`` or ``_some``\ , e.g. ``_ some_variable``\ , ``some_function``\ , ``some_interaction``\ , ...
  When writing your model, you should change these.
* Remember to adapt the plotting and testing functions such that they belong to your model.
* Have a look at the :ref:`impl` page for more information.

.. todo:: 🚧 This section should be expanded.


Some Final Remarks and Advice
-----------------------------

Inspiration from other models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you want to learn more about the capabilities of Utopia and what models can look like, we *strongly* recommend that you have a look at the already implemented models in the ``src/utopia/models`` directory.


``log->debug`` instead of ``std::cout``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you are used to writing C++ code you probably often use ``std::cout`` to print information or to debug your code.

We advise using the functionality of the ``spdlog`` package instead when working with Utopia.
To that end, the ``Model`` base class already provides the ``_log`` member.
Advantages of using a logger instead of directly writing to ``std::cout`` are:

* The output verbosity can be easily controlled via the so-called "log level", without touching any code.
* For a debugging session, the verbosity can be increased, making bug hunting easier.

Which log level should be chosen, though?
As a rough guideline:

* Use ``log->info("Some info")`` for information that is not repetitive, e.g.
  not inside a loop, and contains rather general information.
* Use ``log->debug("Some more detailed info, e.g. for helping you debug")`` for debugging purposes.
* Use the python-like formatting syntax:
  ``log->debug("Some parameter: {:.3f}", param)`` to output parameters.

More information about how to use ``spdlog``, what functionality is provided, and formatting schemes can be found `in their documentation <https://github.com/gabime/spdlog>`_.

Monitoring
^^^^^^^^^^
Utopia models have the ability to communicate the model's current state to the frontend, e.g. the number of cells with a certain state, or the density of agents.
This is done only after a certain ``monitor_emit_interval``\ , to save computing resources.
As this data is communicated to the frontend via ``std::cout``, try to keep it to the bare minimum.

For an example, check out the ``monitor`` function of the ``CopyMe`` model.



Finished!
---------
Congratulations, you have built a new model! :)

Your next guide will be the :ref:`model requirements <dev_model_requirements>`.
It contains information about which requirements your code must fulfill so that it can be accepted as a model within Utopia, i.e. that it can be merged into Utopia's ``master`` branch.

Have fun implementing your own Utopia model! :)

.. note::

    Once you have your own model implemented, you might want to consider to :ref:`couple two or more models <impl_nested>`.
