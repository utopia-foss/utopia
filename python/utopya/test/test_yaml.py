"""Tests the yaml module"""

import math
from typing import Any

import pytest

import ruamel.yaml
from ruamel.yaml.constructor import ConstructorError

import utopya.yaml


# -----------------------------------------------------------------------------

def test_scalar_node_to_object():
    """Test the _scalar_node_to_object helper function

    NOTE It is important here to not only test with a tagged node, but also
         with untagged nodes, as the
    """
    def to_node(d: Any, *, tag=None, set_tag: bool=True):
        """Given some data, represents it as a node and allows to remove the
        tag information.
        """
        node = utopya.yaml.yaml.representer.represent_data(d)
        if set_tag:
            node.tag = tag
        return node

    loader = utopya.yaml.yaml.constructor
    to_obj = utopya.yaml._scalar_node_to_object

    to_test = (
        # Null
        (None, None),
        ("~", None),
        ("null", None),

        # Boolean
        (True, True),
        ("true", True),
        ("TrUe", True),
        ("y", True),
        ("yEs", True),
        ("oN", True),

        (False, False),
        ("false", False),
        ("FaLsE", False),
        ("n", False),
        ("nO", False),
        ("oFf", False),

        # Int
        (123, 123),
        ("0", 0),
        ("1", 1),
        ("-123", -123),
        ("+123", 123),

        # Float
        # (1.23, 1.23),  # FIXME ... some upstream error here!
        ("1.23", 1.23),
        ("-2.34", -2.34),
        ("1e10", 1e10),
        ("1.23e-10", 1.23e-10),
        (".inf", float("inf")),
        ("-.inf", -float("inf")),
        (".NaN", float("nan")),
        (".nan", float("nan")),
        ("nan", float("nan")),

        # String
        ("", ""),  # not null!
        ("some string", "some string"),
        ("123.45.67", "123.45.67"),

        # Exceptions
        ([1,2,3], ConstructorError),
        (dict(foo="bar"), ConstructorError),
    )

    for s, expected in to_test:
        print(f"Case: ({repr(s)}, {repr(expected)})")
        node_tagged = to_node(s, set_tag=False)
        node_untagged = to_node(s)
        print(f"\t{node_tagged}")

        if isinstance(expected, type) and issubclass(expected, Exception):
            print(f"\t… should raise {expected}")

            with pytest.raises(expected):
               to_obj(loader, node_tagged)
            with pytest.raises(expected):
               to_obj(loader, node_untagged)

        else:
            print(f"\t… should be converted to:  "
                  f"{type(expected).__name__} {repr(expected)} ...")

            actual_tagged = to_obj(loader, node_tagged)
            actual_untagged = to_obj(loader, node_untagged)

            # Check type and value
            assert type(actual_tagged) is type(expected)
            assert type(actual_untagged) is type(expected)

            if isinstance(expected, float) and math.isnan(expected):
                assert math.isnan(actual_tagged)
                assert math.isnan(actual_untagged)
            else:
                assert actual_tagged == expected
                assert actual_untagged == expected
