.. _coding_guidelines:

Coding Guidelines
=================

General
-------

The **language** of Utopia code and documentation should be American English (to be consitent with naming like 'neighbor' vs 'neighbour').

Python
------
We use Python >= 3.7.

Orientation style guide: `PEP 8 <https://peps.python.org/pep-0008/>`__
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* This is the basic orientation. If you are unsure how to write things, look it up here.
* We do not need to be 100% strict on this; the main purpose is: **produce readable and well-documented code.**
* If you want, you can **install a Linter** to help you adhere to the style. For Sublime Text, there is the ``pycodestyle`` package, `see here <https://github.com/SublimeLinter/SublimeLinter-pycodestyle>`_.
* Most important points (these we should stick to!):

  * `A foolish consistency is the hobgoblin of little minds <https://peps.python.org/pep-0008/#a-foolish-consistency-is-the-hobgoblin-of-little-minds>`_
  * *Definitely* use **4 spaces** instead of a single tab stop. Modern code editors can be configured to perform this replacement.

    * In Sublime Text, the options are called: ``translateTabsToSpaces``\ , ``tabSize``\ , and ``useTabStops`` see `here <http://www.sublimetext.com/docs/indentation>`_.

  * String styles (more specific than PEP 8):

    * use single quotes for strings that are parameters, e.g. ``func(mode='auto')``
    * use double quotes for non-parameter strings, e.g. ``log.debug("Something went wrong: %s", bla)``
    * use triple *double quotes* for docstrings

  * Don't use spaces around the '=' sign when used to indicate a keyword argument or a default parameter value.
  * Do *not* align assignments' equal signs. It looks pretty at first, but is terrible to get consistent and to maintain.
  * Compound statements (multiple statements on the same line) are generally discouraged.
  * Regarding comments:

    * Comments that contradict the code are worse than no comments. Always make it a priority to keep the comments up-to-date when the code changes!
    * Avoid comments that state the obvious.
    * Use inline comments sparingly.

  * Stick to the `naming styles <https://peps.python.org/pep-0008/#descriptive-naming-styles>`_ of variables.

Docstrings
~~~~~~~~~~

In order to auto-generate a documentation (using the docstrings in the Python code), we need to adhere to a specific style. For creating the documentation, ``Sphinx`` is used.

For the coding style of the docstring, the `Google Style Python Docstring <https://www.sphinx-doc.org/en/master/usage/extensions/example_google.html>`_ style seems most readable and simple to write.
Additionally, the `sphinxcontrib-napoleon <https://sphinxcontrib-napoleon.readthedocs.io/en/latest/index.html>`_ extension allows parsing the type annotations, further simplifying writing and maintenance of docstrings.

Type Hinting
~~~~~~~~~~~~

Use it where applicable. Stay true to Duck Typing though!


C++
---

We use C++17. For very recent coding guidelines on the new C++ standard(s), refer to
`CppCoreGuidelines <https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md>`_ and
`Scott Meyers: Effective Modern C++ (2014) <https://moodle.ufsc.br/pluginfile.php/2377667/mod_resource/content/0/Effective_Modern_C__.pdf>`_.

We will mostly stick to the STL library.
Its complete reference can be found on `cppreference.com <http://en.cppreference.com/w/>`_.

Style Guidelines
~~~~~~~~~~~~~~~~

C++ lacks concrete style guides such as Python's `PEP 8 <https://peps.python.org/pep-0008/>`__.
However, PEP 8 actually offers great instructions on how to write code *in general*. Its instructions on code layout are especially useful.
Basically, most of the instructions in the above Python section also apply here.

* Keep in mind that other people have to read and understand your code.

  * group your code into self-explanatory logic blocks (2–10 lines)
  * leave an empty line after every block
  * add explanatory comments before every block

It should be stressed that the **main goal** is to have *readable* code.
Still, below are a few (partially harsh restrictions) we would like to impose on C++ code to keep some level of consistency:

*
  Indentation:

  * use **spaces** instead of tabs
  * **one indentation level is represented by 4 spaces**
  * ``private``\ , ``protected``\ , and ``public`` use the enclosing indentation level
  * CPP flags are not indented at all or use the enclosing indentation level

*
  Scopes (\ ``{ }``\ ):

  * every new scope creates an indentation, except ``namespace`` scopes and functions that are defined in one line
  * scope closure ``}`` is a single line, except for one-line-functions
  * scope closures of namespaces (or far away scope openings) should be denoted with a ``// namespace foo`` comment preceding the ``}``

*
  Whitespace:

  * leave a whitespace after any C++ keyword, before any scope opening, and before a function's parameter list

*
  Naming:

  * use ``lower_case_with_underscores`` for variables, class members, methods, and functions
  * begin class member variable names with an underscore ``_``
  * use ``CapitalizedWords`` for classes, structs, and typedefs

*
  Object initialization:

  * initialize objects with an assignment ``=`` if no special constructor should be called
  * use brace-initialization only if you instantiate class members directly, *or not at all*
  * use regular function calls ``()`` for calling another than the default constructor


A note regarding Auto-Formatters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You *may* want to use auto-formatters to help you with all the formatting stuff. They can be very useful if you don't want to be bothered with the tedious task of keeping code consistent.

**However,** take care that your formatter does not change the code formatting of a whole file. In these cases, the ``git diff`` becomes huge and makes it a burden for anyone wanting to review your code. If you accidentally introduce changes like these, you will have to remove them before anyone can start reviewing them, which will just be additional work.

For these cases, it makes sense to configure the formatter to only be invoked on selected code sections. We don't want to start an Auto-Formatter war on the Utopia code base... ;)


Doxygen Documentation
~~~~~~~~~~~~~~~~~~~~~

We use `doxygen <https://www.doxygen.nl/index.html>`_ for automatically creating a code documentation. We advertise the C++ commenting style:

* Start the documentation comment before the documented object with ``///``. State the ``brief`` description right away in one line.
* Add the detailed description after ``/**`` in the following lines. Start every line with an asterisk ``*`` and align the center asterisks as well as the line starts. This avoids 'glued' words in the documentation later on due to missing whitespaces.
* Close the comment with ``*/`` in a new line.
* Use the backslash style for doxygen keywords: ``\param``\ , ``\return``
* Avoid redundant keywords like ``\brief``.
* For short comments, you can append a brief description. This is useful for class members.
* Also see the documentation entry on :ref:`writing documentation <cpp_doc>`.

Example:

.. code-block:: c++

    /// An object representing a cell of a CA
    /** \tparam State State type of this object
     */
    template<typename State>
    class Cell {
    private:
        /// State storage
        State _state;

        /// Some member variable
        bool _some_member;

    public:
        /// Construct a new cell with a certain state
        /** This is really just a lengthy comment to demonstrate
          * how a docstring should look like.
          *
          * \param state State of the new cell
          * \return Well, what does a constructor return?
          */
        Cell (const State& state)
        :
            _state(state),
            _some_member(false)
        {
            // this is just a regular comment
        }
    };
