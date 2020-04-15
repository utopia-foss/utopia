"""A test file that is used to implement usage examples which are then
literal-included into the documentation. This makes sure the usage examples
actually work.

Each of the includes has a start and end string:

    ### Start -- example_name
    # ... to be included code here
    ### End ---- example_name

It can be included into the sphinx documentation using:

    .. literalinclude:: ../python/utopya/test/test_doc_examples.py
        :language: python
        :start-after: ### Start -- example_name
        :end-before:  ### End ---- example_name
        :dedent: 4

NOTE Care has to be taken to choose the correct relative path!

There is the option to remove indentation on sphinx-side, so no need to worry
about that.

Regarding the naming of the tests and example names:

    * Tests should be named ``test_<doc-file-name>_<example_name>``
    * Example names should be the same, unless there are multiple examples to
      be incldued from within one test, in which cases the separate examples
      should get a suffix, i.e. <doc-file-name>_<example_name>_<some-suffix>

In order to let the tests be independent, even for imports, there should NOT
be any significant imports on the global level of this test file!
"""

from pkg_resources import resource_filename

import pytest
import numpy as np

import utopya
from utopya.tools import load_yml


# Fixtures --------------------------------------------------------------------


# -----------------------------------------------------------------------------

def test_faq_frontend_yaml_tags():
    """Tests the YAML tag examples given in faq/frontend.rst"""
    # NOTE literalinclude is done from that file rather than from here
    path = resource_filename("test", "cfg/doc_examples/faq_frontend.yml")
    cfg =load_yml(path)

    # Make some assertions for the more complex examples
    g = cfg['yaml_tags_general']
    assert g['fourtytwo'] == 42.
    assert g['seconds_per_day'] == 60 * 60 * 24
    assert g['powers'] == 2**10
    assert g['parentheses'] == (2 + 3) * 4 / (5 - 6)
    assert g['exp_notation'] == (2.34 / 3.45) * 1e-10

    assert g['list_foo'] == list(range(10))
    assert g['list_bar'] == [4, 6, 8, 10, 12, 14, 16, 18]
    assert g['list_fancy'] == [10, 20, 25, 30, 40, 50, 60, 70, 75, 80, 90, 100]
    assert g['list_nested'] == [0, 2, 4, 6, 8, 10, 20, 40, 60, 80]

    assert g['will_be_true'] is True
    assert g['will_be_false'] is False

    p = cfg['yaml_tags_python_only']
    assert np.isnan(p['some_nan'])
    assert p['some_inf'] == np.inf
    assert p['some_ninf'] == - np.inf

