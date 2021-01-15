.. _impl_step_by_step:

Step-by-step Guide
==================

After having worked through the :doc:`README <../../README>`, the
:ref:`tutorial <tutorial>`, and the :ref:`workflow <dev_workflow>`, this guide is for creating a new model in Utopia.

.. note::

  You **can** use the `Utopia docker image <https://hub.docker.com/r/ccees/utopia>`_ for model development.
  Follow the instructions given there to find out how.

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

⚠️ In the following, you will need to replace all mentions of ``CopyMe`` with the name of your own model.


Choosing a name for your model
------------------------------
This is the point where you should decide on the name of your new Utopia model.

For the purpose of this guide, we assume you want to implement a model called ``MyFancyModel``.
Probably, you will give it a more suitable name.
So, keep in mind to replace every ``MyFancyModel`` below with the actual model name.

.. note::

    Utopia has a naming convention for models.
    Your model name should consist of words that start with Capital Letters and are ``DirectlyConcatenatedWithoutSeparatingSymbols``.

    Also, you should not include the ``Model`` string into the name, e.g.: you *should* name your model ``ForestFire`` rather than ``ForestFireModel``
    (so ``MyFancyModel`` is actually not the best example ;) ).


Setting Up The Infrastructure
-----------------------------
Ok, let's get started by setting up the model infrastructure.
The following will involve a lot of copying and renaming, so make sure to pay special care.

.. hint::

    The Utopia CLI provides a command to do all the copying, renaming of files, and refactoring of file content:

    .. code-block:: bash

        (utopia-env) $ utopia models copy ModelToCopy --new-name MyFancyModel

    If you let the CLI do this step for you, most of the steps below are already covered.
    It still makes sense to go through the individual steps and read the remarks, such that you get an idea of what the CLI tool actually did.

    See ``utopia models copy --help`` for more usage information.

    **For CCEES group members:** to copy to the ``utopia/models`` repository, the target project name needs to be ``UtopiaModels``.
    If this does not work, make sure that the ``utopia/models`` repository on your machine is up-to-date with the latest master and you invoked ``cmake ..`` after the update.


To avoid problems, go through the following points from top to bottom, and first read the entire instructions for one step before starting to carry it out.


1. Move to the ``src/utopia/models`` directory inside the ``utopia`` repository.

2. Copy the ``CopyMe`` directory and paste it in the same directory.

    .. note::

        **Important!** If you are a CCEES group member, you should **not** paste into the ``src/utopia/models`` directory in the ``utopia`` repository (which holds the framework code), but into the corresponding ``src/models`` directory inside the group-internal `models repository <https://ts-gitlab.iup.uni-heidelberg.de/utopia/models>`_.

3. Rename the copied directory to ``MyFancyModel`` (or rather, your chosen
   name).

4. Rename all the files inside of the newly created directory such that all
   occurrences of ``CopyMe`` are replaced by ``MyFancyModel``.

  - You can do so by using the `parameter expansion capabilities <http://wiki.bash-hackers.org/syntax/pe>`_ of BASH: inside your model directory, call

  .. code-block:: bash

    for file in CopyMe*; do mv $file ${file/CopyMe/MyFancyModel}; done

5. Tell Utopia that there is a new model, e.g. include your model in the
   Utopia CMake build routine:

  - In ``src/utopia/models/``, you will find a ``CMakeLists.txt`` file. Open it and let
    CMake find your model directory by including the command:
    ``add_subdirectory(MyFancyModel)``.
  - In ``src/utopia/models/MyFancyModel/``, there is another ``CMakeLists.txt`` file.
    Open it and change the line ``add_model(CopyMe CopyMe.cc)`` to
    ``add_model(MyFancyModel MyFancyModel.cc)``. With this command, you are telling
    CMake to keep track of a new model.

6. Open the file ``MyFancyModel.cc`` in the ``src/utopia/models/MyFancyModel/``
   directory and do the following:

  - Throughout the file, replace all ``CopyMe``'s by ``MyFancyModel``'s.

7. Open the file ``MyFancyModel.hh`` in the ``src/utopia/models/MyFancyModel/``
   directory and do the following:

  - Throughout the file, replace all ``CopyMe``'s by ``MyFancyModel``'s.
  - Throughout the file, replace all ``COPYME``'s by ``MYFANCYMODEL``'s.

8. Open the ``MyFancyModel_plots.yml`` and ``MyFancyModel_base_plots.yml`` files in the ``src/utopia/models/MyFancyModel/`` directory and do the following:

  - Throughout the files, replace all ``CopyMe``'s by ``MyFancyModel``'s.

9. Open the file ``MyFancyModel_cfg.yml`` in the ``src/utopia/models/MyFancyModel/``
   directory and do the following:

  - Throughout the file, replace all ``CopyMe``'s by ``MyFancyModel``'s.

Now check if everything works as desired. For that, follow
these steps

10. Enter the ``build`` directory and run ``cmake ..``.
11. Check that the CMake log contains ``Registered model target: MyFancyModel``.
12. Now execute ``make MyFancyModel``.

  * Are there errors? Check above that you adjusted everything as
    described.
  * Building succeeds? Congratulations! 🎉

13. Use the command line interface to run the model:

  .. code-block:: bash

     cd build
     source ./activate
     utopia run MyFancyModel

If everything works, let's continue with setting up the
testing and plotting framework.

The Python Testing Framework
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can set up a simple Python testing framework in the following way:

14. Move to the ``python/model_tests`` directory.
15. Copy the ``CopyMe`` directory and rename it to ``MyFancyModel``. Make sure
    that there is a file named ``__init__.py`` inside the directory.
16. Inside the created ``MyFancyModel`` directory, rename the
    ``test_CopyMe.py`` file to ``test_MyFancyModel.py``.
17. Open the ``test_MyFancyModel.py`` file and replace all ``CopyMe``'s
    by ``MyFancyModel``'s.

In this ``test_MyFancyModel.py`` file you can add tests to your model.
You have the full capabilities of `pytest <https://pytest.org>`_ available plus
the ``utopya.testtools`` module (as exemplified in the ``CopyMe`` model tests.)

.. note::

  Remember to remove the provided example tests if you remove unneeded parts
  of the former ``CopyMe`` model. Otherwise, you will get error messages when
  running the model.


Custom Model Plots
^^^^^^^^^^^^^^^^^^
As you saw in the :ref:`tutorial <tutorial>`, it is possible to have custom model plots tailored to the data your model is producing.
You can set them up in the following way:

18. Move to the ``python/model_plots`` directory.
19. Copy the ``CopyMe`` directory and rename it to ``MyFancyModel``.
    Make sure that there is a file named ``__init__.py`` inside the directory.

The ``*_plots.yml`` files you copied alongside the model configuration control
the behavior of the plotting framework. In the ``MyFancyModel_plots.yml`` file,
you can specify which plots are to be performed automatically.

The ``state.py`` script is provided to show you how a model specific plotting
script could look like.
In ``generic.py`` you see some examples of generic plotting functions which can
be used in combination with Utopia's :ref:`data transformation and selection
framework <external_plot_creator_DAG_support>`.

When starting to implement more plots, you should definitely have a look at
the :ref:`detailed plotting documentation <eval_plotting>`!

.. note::

    Once you change parts of the former ``CopyMe`` model code, the plots might
    break and you might get errors during plot creation. To alleviate them,
    either adapt the plotting functions, remove them, or temporary disable
    them in the plot configuration (using ``enabled: false``) until you have
    adapted them.



Adapting your code
------------------
Depending on what model you want to implement, you will need to delete or
adapt some provided functions. So, feel free to remove anything you do not
need.

* All variables, functions, etc. that are just there to show how you would use and implement them are denoted with the prefix ``some_`` or ``_some``\ , e.g. ``_ some_variable``\ , ``some_function``\ , ``some_interaction``\ , ...
  When writing your model, you should change these.
* Remember to adapt the plotting and testing functions such that they belong to your model.


Some Final Remarks and Advice
-----------------------------

Inspiration from other models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you want to learn more about the capabilities of Utopia and what models can look like, we recommend that you have a look at the already implemented models in the ``src/utopia/models`` directory.


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



.. _model_coupling:

Coupling Models - the Post-Model Era
---------------------------------------

.. note::

    This is an advanced feature.
    Only couple models when each of them has been tested individually.

Once you have your own model implemented, you might want to consider to couple two or more models.
Doing so is explicitly allowed in Utopia, which provides the functionality to couple even complicated model hierarchies.
Every model is placed one level below its parent model (with the `pseudo parent` at the top), and it passed its parent model at initialization.
Hence, the child model is a member of the parent model and the configuration is passed through the parent.

Operating coupled models usually requires a couple of additional thoughts:

* The parent model has to ``iterate`` or ``run`` the child model as per your design; this can be at any time, in parallel, faster or slower.
* For every model, the ``run()`` command includes the iteration until maximum time and three additional operations:

  #.
    The ``prolog``.
    A function that is to be called before the first iteration of this model.
    Its default function includes the writing of the initial state.
  #.
    The ``epilog``.
    A function that is called after the last iteration of this model.
    Ideally it should be called directly after the last iteration, though this is not a requirement.
    Check with the model's documentation.
  #.
    The `breakpoint`.
    The model may receive a signal to stop iteration, e.g. due to a break condition or the user interrupting the simulation run.
    Upon that signal, the ``stop_now`` flag is set to ``true``, indicating that the iteration should stop and the model should shut down.
    A grace period (default: 2s, configurable via frontend) is given; after that, the model process is killed, which may lead to loss of data.
    If – for special reasons – a system of coupled models needs to perform a specific task at the breakpoint, the flag may be queried using ``this->stop_now.load()``.
    Be aware that time-intensive tasks should *not* be carried out after the breakpoint; the aim is to swiftly take down the model object.
    Also note that this flag is not part of the public interface and may change unexpectedly.

  These operations must be handled manually if the child-model is only iterated.
  That means, call the `prolog` before the first iteration, call the `epilog` after the last iteration, and set a breakpoint if you are performing several iterations in a row.
  Note that the maximum time is equal for all models.
  However, by iterating a model can exceed the maximum time.

For an example, see the :ref:`Environment model <model_Environment>`, that is intended to be used as a child-model and includes a guide how to use it.
