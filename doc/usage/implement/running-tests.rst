.. _running_tests:

Running Tests
*************

Utopia contains unit tests to ensure consistency by checking if class members
and functions are working correctly. This is done both for the C++ and the
Python code.
The tests are integrated into the GitLab Continuous Integration pipeline,
meaning that tests are run upon every push to the project and failing tests
can be easily detected.
Tests can also be executed locally, to test a (possibly altered) version of
Utopia *before* committing and pushing changes to the GitLab.

To build all tests, run the following commands:

.. code-block:: bash

    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j4 build_tests_all

This first sets the ``Debug`` build type as described `in the README <../../README.html#build-types>`_ and
then builds all tests.
The ``-j4`` argument specifies that four test targets can be built in parallel.
It makes sense to adjust this to the number of processors you want to engage
in this task.

To then carry out the tests, call the corresponding ``test_``-prefixed target:

.. code-block:: bash

    make -j4 test_all


Test Groups
===========

Usually, changes which are to be tested are concentrated in a few files, which
makes running all tests with ``test_all`` time-consuming and thus inefficient.
We therefore provide grouped tests, which relate only to a subset of tests.
A group of tests can be built and invoked individually by calling:

.. code-block:: bash

    make build_tests_<identifier>
    make test_<identifier>


Replace ``<identifier>`` by the appropriate testing group identifier from the
table below.

==================== ==========================================================
**Identifier**       **Test description**
-------------------- ----------------------------------------------------------
 ``core``             Model infrastructure, managers, convenience functions …
 ``dataio``           Data input and output, e.g. HDF5, YAML, …
 ``backend``          Combination of ``core`` and ``dataio``
 ``model_<name>``     The C++ model tests of model with name ``<name>``
 ``models``†          The C++ and Python tests for *all* models
 ``models_python``†‡  All python model tests (from ``python/model_tests``)
 ``utopya``†‡         Tests for ``utopya`` frontend package
 ``all``              All of the above. (Go make yourself a hot beverage when invoking this.)
==================== ==========================================================

.. note::

    * Identifiers marked with '†' require all models to be built (by invoking
      ``make all`` before running these tests).
    * Identifiers marked with '‡' do *not* have a corresponding ``build_tests_*``
      target.
    * The ``build_tests_`` targets give you more control in scenarios where you want
      to test *only* building.


Running Individual Test Executables
===================================

Each *individual* test also has an individual build target, the names of which
you see in the output of the ``make build_tests_*`` command.
For invoking the individual test executable, you need to go to the
corresponding build directory, e.g. ``build/tests/core/``, and run the executable
from that directory, as some of the tests rely on auxiliary files which are
located relative to the executable.

For invoking individual *Python* tests, there are no targets specified.
However, `pytest <https://docs.pytest.org/en/stable/usage.html>`_ gives you control over which tests are invoked:

.. code-block:: bash

    cd python
    python -m pytest -v model_tests/{model_name}             # all tests
    python -m pytest -v model_tests/{model_name}/my_test.py  # specific test file

Tests for individual ``utopya`` modules need to be run through the ``python/utopya/run_test.py`` executable rather than through ``pytest`` directly:

.. code-block:: bash

    cd python/utopya
    python run_test.py -v test/{some_glob}                   # selected via glob


.. note::
    For all of the above, make sure you entered the virtual environment and the required executables are all built; call ``make all`` to make sure.
    See ``pytest --help`` for more information regarding the CLI.


Evaluating Test Code Coverage
=============================

Code coverage is useful information when writing and evaluating tests.
The coverage percentage is reported via the GitLab CI pipeline.
It is the mean of the test coverage for the Python code and the C++ code.

For retrieving this information on your machine, follow the instructions below.

Python code coverage
====================

The Python code coverage is only relevant for the ``utopya`` package.
It can be analyzed using the `pytest-cov <https://github.com/pytest-dev/pytest-cov>`_
extension for ``pytest``, which is installed into the ``utopia-env`` alongside the
other dependencies.
When running ``make test_utopya``, the code coverage is tracked by default and
shows a table of utopya files and the lines within them that were *not*
covered by the tests.

If you would like to test for the coverage of some specific part of ``utopya``,
adjust the test command accordingly to show the coverage report only for one
module, for example ``utopya.multiverse``:

.. code-block:: bash

    (utopia-env) $ cd python/utopya
    (utopia-env) $ python run_test.py -v test/test_multiverse.py --cov=utopya.multiverse --cov-report=term-missing



C++ code coverage
=================

1. **Compile the source code with code coverage flags.**

Utopia provides the CMake configuration option ``CPP_COVERAGE`` for that purpose,

.. code-block:: bash

    cmake -DCPP_COVERAGE=On ..

This will add the appropriate compiler flags to all tests
and models within the Utopia framework. Notice that code coverage is
disabled for ``Release`` builds because aggressive compiler optimization
produces unreliable coverage results.

2. **Execute the tests.**

Simply call the test commands listed in the previous sections.

3. **Run the coverage result utility** `gcovr <https://gcovr.com/en/stable/>`_.

First, install it via ``pip``. If you want your host system to be unaffected, enter the Utopia virtual environment first.

.. code-block:: bash

    pip3 install gcovr

``gcovr`` takes several arguments. The easiest way of using it is moving to
the build directory and executing

.. code-block:: bash

    gcovr --root ../

This will display the source files and their respective coverage summary
into the terminal. You can narrow down the report to certain source code
paths using the ``--filter`` option and exclude others with the ``--exclude``
option.

``gcovr`` can give you a detailed HTML summary containing the color coded
source code. We recommend reserving a separate directory in the build
directory for that matter:

.. code-block:: bash

    mkdir coverage
    gcovr --root ../ --html --html-details -o coverage/report.html

Note that the C++ code coverage can also be evaluated when using the Python
test framework to run the tests, because the information is gathered directly
from the executable.
This makes sense especially for the model tests, where it is sometimes more
convenient to test the results of a model run rather than some individual part
of it.
