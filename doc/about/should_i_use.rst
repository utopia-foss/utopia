.. _should_i_use:

Should I Use Utopia?
====================

Utopia is for you, if

- You are interested in working with one of the many pre-implemented models for your own purposes, *or*
- You are a researcher looking for a comprehensive modelling tool to investigate complex processes, using e.g. network-based or grid-based models.
- You have knowledge both of C++ and Python.

You might be asking yourself: "Why not use something like `NetLogo <https://ccl.northwestern.edu/netlogo/index.shtml>`_?"
Here's why:

- Utopia is written in modern C++ and Python, and allows developers to exploit the full potential of both programming languages.
- Utopia offers tools to perform parameter sweeps of models and efficiently analyze their high-dimensional data outputs.
- Utopia includes a multithreading interface and can execute simulations concurrently on distributed cluster nodes.
- Utopia supports and encourages software engineering workflows, including version control, unit testing, and more.

On the other hand, you should *not* use Utopia, if

- Programming and using the command line is not your thing; *Utopia embraces flexibility for programmers.*
- You want a library with a simple and concise feature set; *Utopia is feature-rich and extensive.*
- You want a *very* quick solution; *Utopia has a steep learning curve.*

.. admonition:: Do I have to use C++?

    If you do not feel comfortable with programming in C++, you can still benefit from many features of Utopia via the (standalone) Utopia frontend `utopya <https://gitlab.com/utopia-project/utopya>`_.
