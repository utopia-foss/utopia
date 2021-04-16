"""Tests for cluster-related tools"""

import pytest

import utopya
from utopya._cluster import parse_node_list


# -----------------------------------------------------------------------------

def test_parse_node_list_condensed():
    """Tests the parse_node_list function with different input in "condensed"
    mode.

    Also see the following page for the regex pattern:
        https://regex101.com/r/yGdl2j/1
    """
    test_strings = [
        (
            "node042",
            ["node042"]
        ),
        (
            "m13s0603",
            ["m13s0603"]
        ),
        (
            "node[002,009-012,016]",
            ["node002", "node009", "node010", "node011", "node012", "node016"]
        ),
        (
            "m05s[0204,0402,0504]",
            ["m05s0204", "m05s0402", "m05s0504"]
        ),
        (
            "m14s[0501-0503,0505]",
            ["m14s0501", "m14s0502", "m14s0503", "m14s0505"]
        ),
        (
            "node[002,009-012,016],m14s[0501-0503,0505]",
            ["m14s0501", "m14s0502", "m14s0503", "m14s0505",
             "node002", "node009", "node010", "node011", "node012", "node016"]
        ),
        (
            "m05s[0204,0402,0504],m13s0603,m08s[0504,0604],m14s[0501-0502]",
            ["m05s0204", "m05s0402", "m05s0504",
             "m08s0504", "m08s0604",
             "m13s0603",
             "m14s0501", "m14s0502"]
        ),
    ]

    for node_list_str, expected_node_list in test_strings:
        parsed_node_list = parse_node_list(
            node_list_str,
            mode="condensed",
            rcps=dict(
                num_nodes=len(expected_node_list),
                node_name=expected_node_list[-1],
            ),
        )

        assert parsed_node_list == expected_node_list

        # Make sure it's sorted
        assert sorted(parsed_node_list) == parsed_node_list


    # Test error messages
    with pytest.raises(ValueError, match="has a different length"):
        parse_node_list(
            "node042", mode="condensed",
            rcps=dict(num_nodes=2, node_name="not checked"),
        )

    with pytest.raises(ValueError, match="has a different length"):
        parse_node_list(
            "node[009-012]", mode="condensed",
            rcps=dict(num_nodes=2, node_name="not checked"),
        )

    with pytest.raises(ValueError, match="is not part of"):
        parse_node_list(
            "node[009-044]", mode="condensed",
            rcps=dict(num_nodes=36, node_name="node045"),
        )

    with pytest.raises(ValueError, match="empty node list"):
        parse_node_list("", mode="foo", rcps=None)

    with pytest.raises(ValueError, match="Invalid parser mode"):
        parse_node_list("foo", mode="bad_mode", rcps=None)


