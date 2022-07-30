.. _set_up_models_repo:

Setting up a separate repository for models
===========================================
Working inside a clone or a fork of the framework repository is generally *not* a good idea: it makes updating harder, prohibits efficient version control on the models, and makes it more difficult to include additional dependencies or code.
While acceptable for the first steps in Utopia, it makes sense to set up a separate project repository in which your models can be more easily maintained.

To make setting up such a project repository as easy as possible, we provide a `template project <https://gitlab.com/utopia-project/models_template>`_, which can be copied in a matter of minutes and then adjusted to your needs.
Follow the instructions given *there* to set up your own Utopia models repository.

.. note::

    *This* page details only the anatomy of such a models repository and it is not a guide.
    If you want to know more about how to customize the repository or if you run into trouble, read on below (or return here at a later point).

    If you want to get right into setting up your own project repository, directly navigate to the `template project <https://gitlab.com/utopia-project/models_template>`__ and consult the documentation there.



Anatomy of a Utopia models project
----------------------------------
In essence, what we call a "models repository" here is a separate CMake-based project that implements Utopia models.
However, the coupling goes further than just the C++ implementation: it also includes integration of model tests, model plots, documentation, and the possibility to use GitLab CI/CD to automate test execution.

To make setting up such a project more accessible, we provide the template project, which takes care of most of the difficulties, and we describe the important parts where things might need to be changed.

First, let's go through the directories of the template project, one by one.
Crucial segments are denoted with a ❗.

If you have not done so already, open the `template project repository <https://gitlab.com/utopia-project/models_template>`_ now and navigate to the ``{{cookiecutter.project_slug}}`` folder (it's called that way, because it's a *template* project).


``./`` (root level)
^^^^^^^^^^^^^^^^^^^
The root level of the project holds some important files:

- ❗``CMakeLists.txt`` creates the CMake project, links to Utopia, includes further build system modules that specify dependencies, and defines some convenience targets.
- ``README.md`` provides installation instructions; adapt this to your needs.
- ``.gitlab-ci.yml`` holds the CI/CD configuration; you can delete this if you are not planning on using it.
- ❗``.utopya-project.yml`` file holds *project information* that is read by the utopya :py:class:`~utopya.project_registry.ProjectRegistry`.
  This file is *required* and needs to be in a certain format.

  - You can orient yourself at Utopia's corresponding project file:

    .. toggle::

        .. literalinclude:: ../../../.utopya-project.yml
            :language: yaml

  - The ``project_name`` entry should be the name of *your* project and match the ``CMAKE_PROJECT_NAME``.
  - The ``framework_name`` should always be ``Utopia``.


``.gitlab/``
^^^^^^^^^^^^
This directory holds GitLab-related information like issue and MR templates.
These are useful when collaborating with many people on the project.

If you don't intend on hosting the project on GitLab, this directory can be deleted.

``cmake/`` ❗
^^^^^^^^^^^^^
This can be used to add additional build system modules.
The most important file is ``cmake/modules/UtopiaModelsMacros.cmake``, which defines the dependencies of the models project.

``doc/``
^^^^^^^^
Sets up a sphinx- and doxygen-based documentation infrastructure, where model documentation can be housed.
Sphinx is used for the user manual, doxygen for the C++ documentation.

``docker/``
^^^^^^^^^^^
Sets up docker images that are used for automated testing, e.g. via GitLab CI/CD.
There typically is some form of base image that fulfills the Utopia dependencies.
If there are additional dependencies for the project repository, it might make sense to have a second image that installs those; by creating this second image, CI/CD runtimes and resource use is greatly reduced because dependencies do not have to be installed repeatedly.

``python/`` ❗
^^^^^^^^^^^^^^
This directory hosts the model plots and model tests, which are separate Python modules that are integrated into the plot and test routine by the Utopia frontend package, :py:mod:`utopya`.

However, more importantly, the ``python/CMakeLists.txt`` file will invoke the utopya model registry, which takes care of gathering the relevant information for running your models (the source directory, path to binary, etc).

.. warning::

    Even if you do not plan on using the Python model tests and model plots, **do not delete** the ``python/CMakeLists.txt`` file, which takes care of model registry!

This is also the directory where you can supply :ref:`project-level updates to the Multiverse configuration <config_hierarchy>` or an additional project-level base plot configuration pool:
Simply add the two files and specify their relative paths in the ``.utopya-project.yml`` file.

.. note::

    If you have created your own project from the template, the files and entry in the project info file will already have been created.

.. hint::

    If your project has additional Python dependencies, use ``python/requirements.txt`` to specify them.
    These are automatically installed via the build system and pip.


``src/models/`` ❗
^^^^^^^^^^^^^^^^^^
This is where your model implementations live.

The root-level ``src/models/CMakeLists.txt`` file is used to communicate the existence of these models to the build system.
