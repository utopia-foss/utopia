
A Beginner's Guide for Setting Up a New Model
=============================================

This is the beginner's guide for creating a new model in *Utopia*.

If you go through all the steps, you will end up with a model that can profit from all *Utopia* features and can do ... basically nothing interesting yet.
It is a starting point for your own expedition in the *Utopia* model world.
You will be the one who afterwards defines the rules, the entities etc. of this world.

But before this fun part can start, the framework needs to be set up... To avoid problems, go through the following sections from top to bottom.

.. note::

  The steps in this guide are numbered; if you run into problems and ask for help, you can name the step you got stuck at.

Setting Up The Infrastructure
-----------------------------

We assume that you want to build your own model, called ``MyFancyModel``. Probably, you will give it a more suitable name. So, keep in mind to replace every ``MyFancyModel`` below with the actual model name.

For the setup of the infrastructure, we provide a so-called ``CopyMe`` model, which can be copied and used as basis for your own model; this will be the first step below.
Note that there is also a ``CopyMeBare`` model; it includes *only the bare basics* needed for a model, while the ``CopyMe`` model also includes showcase of some functionality. If you are new to *Utopia*, you should go with the latter; if you are already more proficient, use the former.

1. Move to the ``dune/utopia/models/`` directory inside the ``utopia`` repository.
2. Copy the ``CopyMe`` directory and paste it in the same directory.
3. Rename the copied directory to ``MyFancyModel`` (or rather your chosen name).

  - A remark concerning the naming convention: Your model name should consist of words that start with Capital Letters and are ``DirectlyConcatenatedWithoutSeparatingSymbols``.

4. Rename all the files inside of the newly created directory such that all occurrences of ``CopyMe`` are replaced by ``MyFancyModel``.

  - You can do so by using the `parameter expasion capabilities <http://wiki.bash-hackers.org/syntax/pe>`_ of BASH: Inside your model directory, call

  .. code-block:: bash

    for file in CopyMe*; do mv $file ${file/CopyMe/MyFancyModel}; done

5. Tell *Utopia* that there is a new model, e.g. include your model in the Utopia CMake build routine:

  - In ``dune/utopia/models/``, you find a ``CMakeLists.txt`` file. Open it and let CMake find your model directory by including the command: ``add_subdirectory(MyFancyModel)`` 
  - In ``dune/utopia/models/MyFancyModel/``, there is another ``CMakeLists.txt`` file. Open it and change the line ``add_model(CopyMe CopyMe.cc)`` to ``add_model(MyFancyModel MyFancyModel.cc)``. With this command, you tell CMake that a new model should be kept track of.

6. Open the file ``MyFancyModel.cc`` in the ``dune/utopia/models/MyFancyModel/`` directory and do the following:

  - Throughout the file, replace all ``CopyMe``'s by ``MyFancyModel``'s.

7. Open the file ``MyFancyModel.hh`` in the ``dune/utopia/models/MyFancyModel/`` directory and do the following:

  - Throughout the file, replace all ``CopyMe``\ 's by ``MyFancyModel``\ 's.
  - Throughout the file, replace all ``COPYME``\ 's by ``MYFANCYMODEL``\ 's.

It's time for a little check if everything works as desired. For that, follow the following steps:


8. Enter the ``build-cmake`` directory and run ``cmake ..``
9. Check that the CMake log contains ``Registered model target: MyFancyModel``
10. Now execute ``make MyFancyModel`` ...

  * Are there errors? Hmmm... check above that you adjusted everything as described.
  * Building succeeds? Nice! 🎉

11. Use the command line interface to run the model:

  .. code-block:: bash

     cd build-cmake
     source ./activate
     utopia run MyFancyModel

Hoping that everything went well so far, let's continue with setting up the testing and plotting framework...

The Testing Framework
^^^^^^^^^^^^^^^^^^^^^

You can set up the testing framework in the following way:

12. Move to the ``python/model_tests`` directory
13. Copy the ``CopyMe`` directory and rename it to ``MyFancyModel``. Make sure that there is a file named ``__init__.py`` inside the directory. 
14. Inside the created ``MyFancyModel`` directory, rename the ``test_CopyMe.py`` file to ``test_MyFancyModel.py``.
15. Open the ``test_MyFancyModel.py`` file and replace all ``CopyMe``\ 's by ``MyFancyModel``\ 's.

In this ``test_MyFancyModel.py`` file you can add tests to your model. 

.. note::

  Remember to remove the provided example tests if you remove unneeded parts of the former ``CopyMe`` model. Otherwise, you will get error messages when running the model.


Custom Model Plots
^^^^^^^^^^^^^^^^^^

As you saw in the :doc:`tutorial <tutorial>`, it is possible to have custom model plots which are taylored to the data your model is producing.
You can set them up in the following way:

16. Move to the ``python/model_plots`` directory
17. Copy the ``CopyMe`` directory and rename it to ``MyFancyModel``. Make sure that there is a file named ``__init__.py`` inside the directory.

The ``state.py`` script is provided to show you how a model specific plotting script could look like. Remember to remove it (comment it out) if you start removing or changing parts of the former ``CopyMe`` model code. Otherwise, you will get error messages.

Adapting your code
------------------

Depending on what model you want to implement, you will need to delete or adapt some provided functions. So, feel free to remove anything, you do not need.

* All variables, functions, etc. that are just there to show how you would use and implement them are denoted with the prefix ``some_`` or ``_some``\ , e.g. ``_ some_variable``\ , ``some_function``\ , ``some_interaction``\ , ...
  If you write your model, you should change these.
* Keep in mind to adapt the plotting and testing functions such that they belong to your model.

Some Final Remarks and Advice
-----------------------------

Inspiration from other models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to learn more about the capabilities of Utopia and how models can look like, we recommend that you have a look at `the already implemented models <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia#currently-implemented-models>`_.

``log->debug`` instead of ``std::cout``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you are used to writing ``C++`` code you probably often use ``std::cout`` to print information or to debug your code. We advice you to use the functionality of ``spdlog`` if you work with *Utopia*. This has at least two advantages:

* If you run your model, your information is stored in a ``out.log`` for each universe, so you can have a look at the logger information later.
* If you do big parameter sweeps, your terminal will not be flooded with information.

As a rough guideline:

* Use ``log->info("Some info")`` for information that is not repetitive, e.g. not inside a loop, and contains rather general information.
* Use ``log->debug("Some more detailed info, e.g. for helping you debug")`` 
* Use the python-like formatting syntax: ``log->debug("Some parameter: {:.3f}", param)`` to output parameters.

More information about how to use ``spdlog``, what functionality is provided, and formatting schemes can be found `in their documentation <https://github.com/gabime/spdlog>`_.

Monitoring
^^^^^^^^^^

Utopia models have the ability to communicate the model's current state to the frontend, e.g. the number of cells with a certain state, or the density of agents or the like.
This is done only after a certain ``monitor_emit_interval``\ , to save computing resources. As this data is communicated to the frontend via ``std::cout``, try to keep it to the bare minimum.

For examples, check out the ``monitor`` function of the ``CopyMe`` model.

Finished!
---------

Congratulations, you have build a new model! :)

Your next guide will be the :doc:`<guides/model-requirements>`.
It contains information what requirements your code must fulfill such that it can be accepted as a model within *Utopia*, e.g. that it can be merged into *Utopia*'s ``master`` branch.

Have fun implementing your own *Utopia* model! :) 
