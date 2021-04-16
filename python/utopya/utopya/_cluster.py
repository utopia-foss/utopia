"""This module holds functions used in the Multiverse cluster mode"""

import re
from typing import List



# -----------------------------------------------------------------------------

def parse_node_list(node_list_str: str, *, mode: str, rcps: dict) -> List[str]:
    """Parses the node list to a list of node names and checks against the
    given resolved cluster parameters.

    Depending on ``mode``, different forms of the node list are parsable.
    For ``condensed`` mode:

    .. code-block::

        node042
        node[002,004-011,016]
        m05s[0204,0402,0504]
        m05s[0204,0402,0504],m08s[0504,0604,0701],m13s0603,m14s[0501-0502]
    """
    if not node_list_str:
        raise ValueError("Got empty node list string representation!")

    pattern = ""
    node_list = []

    if mode == 'condensed':
        # See  https://regex101.com/r/yGdl2j/1  for deduction of regex pattern
        pattern = r'((?P<prefix>[\w\d]+)\[(?P<nodes>[\d\-\,\s]+)\]|(?P<node>[\w\d]+)),?'
        matches = re.finditer(pattern, node_list_str)

        for match in matches:
            if match["node"] is not None:
                # Was only a single node, not in condensed form
                node_list.append(match["node"])
                continue

            # Got a condensed match; need to continue parsing it ...
            prefix, nodes = match["prefix"], match["nodes"]

            # Remove whitespace and split
            segments = nodes.replace(" ", "").split(",")
            segments = [seg.split("-") for seg in segments]

            # Get the string width
            digits = len(segments[0][0])

            # In segments, lists longer than 1 are intervals, the
            # others are node numbers of a single node.
            # Expand intervals
            segments = [
                [int(seg[0])] if len(seg) == 1
                 else list(range(int(seg[0]), int(seg[1])+1))
                for seg in segments
            ]

            # Combine to list of individual node numbers
            node_nos = []
            for seg in segments:
                node_nos += seg

            # Need the numbers as strings
            node_nos = [
                "{val:0{digs:}d}".format(val=no, digs=digits)
                for no in node_nos
            ]

            # Now, finally, parse the list
            node_list += [prefix+no for no in node_nos]

    else:
        raise ValueError(
            f"Invalid parser mode '{mode}'! Available: condensed"
        )

    # Have node_list now. Make some consistency checks
    if rcps['num_nodes'] != len(node_list):
        raise ValueError(
            f"The parsed node list ({node_list}) has a different length "
            f"({len(node_list)}) than specified by the `num_nodes` parameter "
            f"({rcps['num_nodes']})! Make sure the string representation of "
            f"the node list ({repr(node_list_str)}) is parsable by the "
            f"selected parser mode '{mode}' "
            f"with the following regex pattern: {repr(pattern)}"
        )

    if rcps['node_name'] not in node_list:
        raise ValueError(
            f"The current node's `node_name` '{rcps['node_name']}' is not "
            f"part of the parsed node list: {', '.join(node_list)}"
        )

    # All ok. Sort and return
    return sorted(node_list)
