Writing Unit Tests in Utopia
****************************

Utopia utilizes the Boost Unit Test Framework for conveniently writing and
executing unit tests. Using it is not required, but highly recommended!

This page will give you a small introduction on how to write a unit test in
Utopia. For advanced usage of the framework, please refer to its
documentation.

.. contents::
   :local:
   :depth: 3

----



What Is a Unit Test?
====================

TLDR: A unit test verifies the usage and operating procedures of single source
code units (paraphrased from
`Wikipedia <https://en.wikipedia.org/wiki/Unit_testing>`_).

Whenever you write some new code, you should create a unit test to verify that
it works. Including the test into the automated CI/CD system ensures that it
will be working as intended after future updates.


.. _good_unit_test:

Writing a *Good* Unit Test
--------------------------

When writing unit tests, sticking to the following high-level principles will help making your tests robust, extensible, and informative:

1. **Granular code testing** — ideally, test cases are as short as possible
2. **Avoid copy-paste** — as with regular code, test code will become hard to maintain otherwise
3. **Useful test debug messages** — if a test fails, one ideally immediately knows where and why

To achieve this, make sure to use the full capabilities of the testing framework you have at hand; for Boost Test, see :ref:`good_boost_test`.

.. note::

    As general rule of thumb, take as much care of writing tests than of writing your "actual" code!


Writing Unit Tests with Boost Test
==================================

The Boost Unit Test Framework is a library supplying convenient macros for
unit testing and is highly customizable. It has a lot of functions, but most
will not be necessary for simple tests.

Programmers only have to supply the test functions or test function templates.
The library itself will wrap everything into a proper executable which can even
take additional arguments.


Most Useful References
----------------------

* `Declaring and Organizing Tests`_
* `Summary of the API for Declaring and Organizing Tests`_
* `Summary of the API for Writing Tests`_


How to Start
------------
Create a new C++ source file. In there, declare your unit test before including
the proper Boost header.

.. code-block:: c++

    #define BOOST_TEST_MODULE my great unit test
    #include <boost/test/unit_test.hpp>

Use the CMake function ``add_unit_test`` to register this source file as unit
test. The function takes the following arguments:

* ``NAME``: The name of the unit test *as registered in CMake*. This is
  independent from the name you declare for the test module in the source file
  (though it makes sense to name them similarly...)
* ``GROUP``: The unit test group for this test. The uppercase group name will
  be prepended to the unit test name. The lowercase group name will be used
  to register the test targets. After building them, you can execute all unit
  tests of this group with the command ``make test_<group>``.
* ``SOURCES``: All source files for this test, like the arguments to
  ``add_executable``. Notice that the macro ``BOOST_TEST_MODULE`` may only be
  defined in **one single source file**.

.. code-block:: cmake

    add_unit_test(NAME my_great_unit_test
                  GROUP core
                  SOURCES file.cc)


Writing a Simple Unit Test
--------------------------
For simply declaring a single test function, use ``BOOST_AUTO_TEST_CASE``.
The first argument to this function is the test case name (no ``string``
quotation marks needed). In there, use the assertion macro ``BOOST_TEST``
for the things you want to check.

.. code-block:: c++

    BOOST_AUTO_TEST_CASE(case1)
    {
        int i = 0;
        BOOST_TEST(i == 0);
        int j = 1;
        BOOST_TEST(i != j);
    }

That's it! At this point, you already have a working unit test. Boost Test will
take care of the rest. In particular, you must not write any ``main`` function
or handle exceptions.

The important thing about ``BOOST_TEST`` is that execution carries on after an
assertion failed. This is used to give users a full report on which tests are
working and which are failing. However, this might lead to undefined behavior.
You can use ``BOOST_REQUIRE``, if further execution after a failing assertion
would not make sense:

.. code-block:: c++

    BOOST_AUTO_TEST_CASE(case2)
    {
        int* i = get_pointer_to_int();
        BOOST_REQUIRE(i != nullptr);
        BOOST_TEST(*i == 0); // (*i) is valid if we reach this point
    }

For more assertion macros, see the `Summary of the API for Writing Tests`_.


.. _boost_test_templates:

Unit Tests on Templates
-----------------------
We often use templated code and must check if it works for different data types
inserted. This can be easily achieved by declaring a test function that takes
several types and is executed for every type seperately. In the function
signature, specify the test case name, the name of the type used inside the
function, and the list of all types which should be used.
For more information, see the Boost Test docs on `template test cases <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/tests_organization/test_cases/test_organization_templates.html>`_.

.. code-block:: c++

    #include <boost/mpl/list.hpp>
    #include <utopia/core/state.hh>

    using AllTypes = boost::mpl::list<int, double>;

    BOOST_AUTO_TEST_CASE_TEMPLATE (case3, ThisType, AllTypes)
    {
        StateContainer<ThisType, true> cont(0);
        BOOST_TEST(cont.state() == 0);
    }

The above code will result in two test cases, one where ``ThisType`` is a
typedef for ``int``, and one where it is for ``double``.

.. note::

    Recent compilers also support specifying the template type list as ``std::tuple``.

.. _unit_test_fixtures:


.. _boost_test_fixture:

Using a Fixture
---------------
Fixtures a standardized objects instantiated for every single test function
execution. Use them to avoid repeating the setup of certain objects within
multiple test functions. A fixture for Boost Test should be a ``struct`` with
public members. These members will be *directly* available within the test
function. You can also define fixtures for entire test suites.
For more information, see the Boost Test docs on `test fixtures <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/tests_organization/fixtures/case.html>`_.

.. code-block:: c++

    // Something we test
    struct Agent {
        int index;
        double value;
    }

    // The fixture
    struct SomeValues {
        int index = 0;
        double value = 1.1;
    }

    // The fixture is instantiated seperately for every function
    BOOST_FIXTURE_TEST_CASE(case3, SomeValues)
    {
        Agent agent({index, value});
        BOOST_TEST(agent.index == index);
        BOOST_TEST(agent.value == value);
    }

.. hint::

    If you find yourself frequently using the same fixture, have a look at ``BOOST_FIXTURE_TEST_SUITE`` (`documentation <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/tests_organization/fixtures/case.html#boost_test.tests_organization.fixtures.case.fixture_for_a_complete_subtree>`_).
    Inside the fixture test suite, you can conveniently use ``BOOST_AUTO_TEST_CASE``.
    This can also be useful if you want to use the templated test cases described above *and* a fixture at the same time.


.. _boost_test_compare_custom_types:

Comparing Custom Types
----------------------
The Boost Test assertion macros can compare all integral types of C++. For
comparing custom types, additional information has to be made available such
that failures can be properly reported. In particular, users have to define
the proper comparison functions and an overload of the ``<<`` stream operator:

.. code-block:: c++

    #include <iostream>

    // Just a strongly-typed int
    struct Int {
        int value;
    };

    // How to compare Int
    bool operator== (const Int& lhs, const Int& rhs)
    {
        return lhs.value == rhs.value;
    }

    // How to report Int in an output stream
    std::ostream& boost_test_print_type (std::ostream& ostr,
                                         Int const& right)
    {
        ostr << right.value;
        return ostr;
    }

    BOOST_AUTO_TEST_CASE(case4)
    {
        Int int_1({4});
        Int int_2;
        int_2.value = 4;
        BOOST_TEST(int_1 == int_2); // Yay, this works now!
    }


.. _boost_test_utils:

Useful Boost Test Utilities
---------------------------
There are a number of utilities that help to implement tests or assertions.

Floating-point Comparisons
^^^^^^^^^^^^^^^^^^^^^^^^^^
Comparing floating-point numbers often requires a tolerance in order to be stable and independent of the specific system a test is run on.

With Boost Test, a tolerance can be defined both on the level of a test case and for individual assertions.
If both are specified, the latter takes precedence over the former, as shown in this example:

.. code-block:: c++

    #define BOOST_TEST_MODULE tolerance
    #include <boost/test/included/unit_test.hpp>
    namespace utf = boost::unit_test;
    namespace tt = boost::test_tools;

    // Test case with updated tolerance setting
    BOOST_AUTO_TEST_CASE(test1, * utf::tolerance(0.00001))
    {
        double x = 10.0000000;
        double y = 10.0000001;
        double z = 10.001;
        BOOST_TEST(x == y); // irrelevant from tolerance
        BOOST_TEST(x == y, tt::tolerance(0.0));

        BOOST_TEST(x == z); // relevant from tolerance
        BOOST_TEST(x == z, tt::tolerance(0.001));
    }

See `the documentation <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/testing_tools/extended_comparison/floating_point.html>`_ for more information.

Collection Comparisons
^^^^^^^^^^^^^^^^^^^^^^
By default, collections are compared via their corresponding comparsion operator.

However, performing **element-wise comparison** can often be useful.
This is simple and straight-forward with Boost Test:

.. code-block:: c++

    #define BOOST_TEST_MODULE boost_test_sequence_per_element
    #include <boost/test/included/unit_test.hpp>
    #include <vector>
    #include <list>
    namespace tt = boost::test_tools;

    BOOST_AUTO_TEST_CASE( test_sequence_per_element )
    {
        std::vector<int> a{1,2,3};
        std::vector<long> b{1,5,3};
        std::list<short> c{1,5,3,4};

        BOOST_TEST(a == b, tt::per_element()); // nok: a[1] != b[1]

        BOOST_TEST(a != b, tt::per_element()); // nok: a[0] == b[0] ...
        BOOST_TEST(a <= b, tt::per_element()); // ok
        BOOST_TEST(b  < c, tt::per_element()); // nok: size mismatch
        BOOST_TEST(b >= c, tt::per_element()); // nok: size mismatch
        BOOST_TEST(b != c, tt::per_element()); // nok: size mismatch
    }

Read more about different ways of comparing collections in the `corresponding documentation <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/testing_tools/extended_comparison/collections.html>`_.




.. _good_boost_test:

Writing A *Good* Boost Test
---------------------------

Following the motivation of the remarks on :ref:`good_unit_test` above, the list below provides information on how to achieve this with the tools provided by Boost Test.

* **Write small tests** and organize them into logical units, so-called *test suites*:

    * Test suites help to provide information on where an error occurred and which test suites belong together. You can regard it as another way of modularization.
    * Use ``BOOST_AUTO_TEST_SUITE``, as explained `here <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/tests_organization/test_tree/test_suite.html#boost_test.tests_organization.test_tree.test_suite.automated_registration>`_.

* **Avoid copy-paste** code by ...

    * ... making use of :ref:`fixtures <boost_test_fixture>`. This will furthermore provide robust procedures for setup and teardown of test cases.
    * ... using :ref:`template test cases <boost_test_templates>`, which allows to easily specify tests for multiple types.

* **Provide useful information upon failure**.

    * Where possible, directly use ``BOOST_TEST``, i.e.: ``BOOST_TEST(a == b)``

        * When doing ``BOOST_TEST(some_bool_evaluating_function(a, b))``, the test output will not be useful as it will only say ``false``.
        * Note that you can also :ref:`compare custom types <boost_test_compare_custom_types>`.

    * There are a multitude of ways to `control test output <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/utf_reference/testout_reference.html>`_.
      For example, with ``BOOST_TEST_CONTEXT``, you can specify a message that is shown when an assertion fails within the context.

        * The context message can inform about the set of parameters that are used for the assertions or that were used to set up the object that is tested in that context.
        * Contexts can also be nested.
        * Read more about contexts `here <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/test_output/test_tools_support_for_logging/contexts.html>`_.

    * ``BOOST_TEST_CHECKPOINT`` and ``BOOST_TEST_PASSPOINT`` help to better locate failure location.

        * This can be useful when a failure occurs not within or near a ``BOOST_*`` statement, but elsewhere.
        * Note that every ``BOOST_*`` statement automatically acts as a passpoint.
        * Read more about failure location `here <https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/test_output/test_tools_support_for_logging/checkpoints.html>`_.




Using ``Utopia::TestTools``
===========================
Utopia provides a set of test tools which make it easier to apply the above.

Using these tools is as simple as including the ``utopia/core/testtools.hh`` header:

.. code-block:: c++

    #define BOOST_TEST_MODULE test my_fancy_feature

    #include <utopia/core/testtools.hh>

    // Use the Utopia::TestTools namespace
    using namespace Utopia::TestTools;

    // +++ Tests +++
    // ... your tests here ...

.. note::

    Refer to the `corresponding doxygen documentation <../../doxygen/html/group___test_tools.html>`_ entries for more detailed information.


The ``BaseInfrastructure`` fixture
----------------------------------
Frequently, tests or models require some kind of logger, random number generator, and some form of configuration.
The ``BaseInfrastructure`` fixture provides these tools.
It can be specialized to the need of the currently used test module using inheritance from the base class:

.. code-block:: c++

    #define BOOST_TEST_MODULE test my_fancy_feature

    #include <utopia/core/testtools.hh>

    // Use the Utopia::TestTools namespace
    using namespace Utopia::TestTools;


    // +++ Fixtures +++

    /// A specialized infrastructure fixture, loading a configuration file
    /** \note If no configuration file is required or available, you can
      *       simply omit the file path. The configuration is then empty.
      */
    struct Infrastructure : BaseInfrastructure<> {
        Infrastructure () : BaseInfrastructure<>("my_test_config.yml") {};
    };


    // +++ Tests +++
    /// Some simple test case using that fixture
    BOOST_FIXTURE_TEST_CASE (test_my_test_with_fixture, Infrastructure)
    {
        // Have all members available directly here: log, rng, cfg, ...
        const auto default_cfg = cfg["defaults"];

        // ...
    }

If desired, the derived class can also be extended to provide more members, just like :ref:`regular fixtures <unit_test_fixtures>`.

.. note::

    **Important:** If you change test configuration files, e.g. the ``my_test_config.yml`` used in the fixture, remember to invoke ``cmake ..`` again, which takes care of copying that file to the directory where the test executables are placed.
    Otherwise, your tests might appear to be failing due to an outdated configuration file.


Testing exceptions
------------------
To test that an exception not only matches a specific type, but also a specific error message, you can use the ``check_exception`` test tool:

.. code-block:: c++

    #define BOOST_TEST_MODULE test my_fancy_feature

    #include <utopia/core/testtools.hh>

    // Use the Utopia::TestTools namespace
    using namespace Utopia::TestTools;

    // +++ Tests +++
    /// Test that some callable invokes the expected exception
    BOOST_AUTO_TEST_CASE (test_my_exception)
    {
        check_exception<std::invalid_argument>(
            [](){
                // Do some stuff ...

                // This will throw:
                some_function_that_expects_a_positive_number(-1);
            },
            "Expected a positive number, got: -1"  // expected error message
        );
    }

The ``match`` string is optional; if it is not given, only the exception type will be checked.
See `the corresponding doxygen documentation <../../doxygen/html/group___test_tools.html>`_ for more information.

.. warning::

    When ``check_exception`` fails, the test output will show the error originating from within ``utopia/core/testtools``, because that's where the ``BOOST_ERROR`` is invoked from.

.. hint::

    If you have trouble pinning down the error location and :ref:`reducing the test case size <good_unit_test>` is *not* an option, you can supply location information by adding ``{__LINE__, __FILE__}`` as last argument:

    .. code-block:: c++

        check_exception<std::invalid_argument>(
            [](){ throw std::invalid_argument("foo"); }, "foo",
            {__LINE__, __FILE__}
        );


.. _unit_test_config_based:

Configuration-based test cases
------------------------------
Sometimes you might desire to repeatedly invoke some callable with a different set of parameters.
Which easier way is there to do this than via a configuration file?
The ``test_config_callable`` function is your friend:

.. code-block:: c++

    BOOST_AUTO_TEST_CASE (test_my_config_callable)
    {
        test_config_callable(
            // Define some callable, ad-hoc, which expects a config node
            [](auto cfg){
                auto foo = get_as<std::string>("foo", cfg);
                auto some_number = get_as<int>("some_number", cfg);

                some_function_that_throws_on_negative_int(some_number);

                // Can do more tests here. Ideally, use BOOST_TEST( ... )
                // ...
            },
            // The YAML mapping that holds _all_ test cases
            cfg["my_test_cases"],
            // Information that is passed on to the test context; use something
            // unique here such that you can pin down the error's origin.
            "My test cases"
        );
    }

The corresponding configuration (here: ``cfg``) should look something like this and can specify whether a certain parameter combination is expected to throw an exception, just like with ``check_exception``:

.. code-block:: yaml

    my_test_cases:
      case1:
        # The parameters that are passed to the callable
        params: {foo: bar, some_number: 42}

      # With these parameters, the callable is expected to throw
      case1_but_failing:
        params: {foo: bar, some_number: -1}
        throws: std::invalid_argument

      # Can optionally also define a string to match the error message
      case1_but_failing_with_match:
        params: {foo: bar, some_number: -1}
        throws: std::invalid_argument
        match: "Expected a positive number but got: -1"

      # More test cases ...
      case2:
        params: {foo: spam, some_number: 23}

      case2_KeyError:
        params: {some_number: 23}
        throws: Utopia::KeyError
        match: foo

Again, see `the corresponding doxygen documentation <../../doxygen/html/group___test_tools.html>`_ of the ``test_config_callable`` function for more information.


.. _Summary of the API for Writing Tests:
    https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/
    testing_tools/summary.html

.. _Declaring and Organizing Tests:
    https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/
    tests_organization.html

.. _Summary of the API for Declaring and Organizing Tests:
    https://www.boost.org/doc/libs/1_72_0/libs/test/doc/html/boost_test/
    tests_organization/summary_of_the_api_for_declaring.html
