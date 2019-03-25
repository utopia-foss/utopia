Writing Unit Tests in Utopia
****************************

Utopia utilizes the Boost Unit Test Framework for conveniently writing and
executing unit tests. Using it is not required, but highly recommended!

This page will give you a small introduction on how to write a unit test in
Utopia. For advanced usage of the framework, please refer to its
documentation.

.. contents::
   :local:
   :depth: 2

What Is a Unit Test?
====================

TLDR: A unit test verifies the usage and operating procedures of single source
code units (paraphrased from
`Wikipedia <https://en.wikipedia.org/wiki/Unit_testing>`_).

Whenever you write some new code, you should create a unit test to verify that
it works. Including the test into the automated CI/CD system ensures that it
will be working as intended after future updates.

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

Unit Tests on Templates
-----------------------
We often use templated code and must check if it works for different data types
inserted. This can easily achieved by declaring a test function that takes
several types and is executed for every type seperately. In the function
signature, specify the test case name, the name of the type used inside the
function, and the list of all types which should be used.

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

.. note:: Recent compilers also support specifying the template type list
          as ``std::tuple``. This is **not supported** on Ubuntu Bionic 18.04.

Using a Fixture
---------------
Fixtures a standardized objects instantiated for every single test function
execution. Use them to avoid repeating the setup of certain objects within
multiple test functions. A fixture for Boost Test should be a ``struct`` with
public members. These members will be *directly* available within the test
function.

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


.. _Summary of the API for Writing Tests:
    https://www.boost.org/doc/libs/1_69_0/libs/test/doc/html/boost_test/
    testing_tools/summary.html

.. _Declaring and Organizing Tests:
    https://www.boost.org/doc/libs/1_69_0/libs/test/doc/html/boost_test/
    tests_organization.html

.. _Summary of the API for Declaring and Organizing Tests:
    https://www.boost.org/doc/libs/1_69_0/libs/test/doc/html/boost_test/
    tests_organization/summary_of_the_api_for_declaring.html
