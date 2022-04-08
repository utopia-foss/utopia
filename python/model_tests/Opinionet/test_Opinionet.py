"""Tests of the output of the Opinionet model"""
import networkx as nx
import numpy as np
import pytest

from utopya.eval.plots._graph import GraphPlot  # FIXME Should not be used here
from utopya.testtools import ModelTest

# Configure the ModelTest class for Opinionet
mtc = ModelTest("Opinionet", test_file=__file__)

# Tests -----------------------------------------------------------------------


def test_static():
    """Test the initialization of the model and the case of static opinions"""

    # Create a Multiverse using the default model configuration
    mv, dm = mtc.create_run_load(from_cfg="static.yml", perform_sweep=True)

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Opinionet"]
        cfg = uni["cfg"]["Opinionet"]
        initial_opinions = data["nw"]["opinion"].isel(time=0)
        final_opinions = data["nw"]["opinion"].isel(time=-1)

        # Check that opinions don't change, as susceptibility = 0
        for init, final in zip(initial_opinions, final_opinions):
            assert init == final

        # Check that the total number of vertices is correct
        num_vertices = cfg["network"]["num_vertices"]
        users = data["nw"]["_vertices"].data

        assert len(users) == num_vertices

        # Check that edge weight data was written for directed graphs
        # Check that edge weights don't change due to rewiring = false
        # Check that edge weights are normalised probabilities
        if cfg["network"]["directed"] == True:
            initial_weights = data["nw"]["edge_weights"].isel(time=0)
            final_weights = data["nw"]["edge_weights"].isel(time=-1)

            # No rewiring due to large tolerance
            for init, final in zip(initial_weights, final_weights):
                assert init == final

            edge_weights = np.append(initial_weights, final_weights)

            for weight in edge_weights:
                assert 0 <= weight <= 1 and not np.isnan(weight)

            assert np.sum(edge_weights) == 2 * num_vertices

        # For continuous opinion space: check opinions are uniformly distributed
        # over opinion space.
        if cfg["opinion_space"]["type"] == "continuous":
            op_space = cfg["opinion_space"]["interval"]
            op_space_center = 0.5 * (op_space[1] + op_space[0])

            assert op_space[0] <= np.min(initial_opinions)
            assert np.max(initial_opinions) <= op_space[1]
            assert (
                op_space_center - 0.05
                < np.mean(initial_opinions)
                < op_space_center + 0.05
            )

        # For discrete opinion space: check all opinions are integer values in
        # the opinion space
        elif cfg["opinion_space"]["type"] == "discrete":
            op_space = list(range(cfg["opinion_space"]["num_opinions"]))
            op_vals = list(set(initial_opinions.data))

            assert op_space == op_vals


def test_single_revision():
    """Test a single revision step"""

    mv, dm = mtc.create_run_load(
        from_cfg="single_revision.yml", perform_sweep=True
    )

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Opinionet"]
        cfg = uni["cfg"]["Opinionet"]
        initial_opinions = data["nw"]["opinion"].isel(time=0)
        final_opinions = data["nw"]["opinion"].isel(time=-1)
        opinion_space = cfg["opinion_space"]
        susceptibility = cfg["susceptibility"]
        tolerance = cfg["tolerance"]

        # Check opinion did not leave opinion space
        # Check that opinion did not change by more than tolerance * susceptibility
        # except for discrete Deffuant, where it should not change by more than
        # the tolerance
        if opinion_space == "continuous":
            op_space = cfg["opinion_space"]["interval"]

            assert op_space[0] <= np.min(initial_opinions)
            assert np.max(initial_opinions) <= op_space[1]

            for init, final in zip(initial_opinions, final_opinions):
                assert abs(init - final) <= susceptibility * tolerance

        elif opinion_space == "discrete":
            op_space = list(range(cfg["opinion_space"]["num_opinions"]))
            op_vals = list(set(final_opinions.data))

            assert op_space == op_vals

            if cfg["interaction_function"] != "Deffuant":
                for init, final in zip(initial_opinions, final_opinions):
                    assert abs(init - final) <= susceptibility * tolerance
            else:
                for init, final in zip(initial_opinions, final_opinions):
                    assert abs(init - final) <= tolerance


def test_dynamics():
    """Test the general dynamics"""

    mv, dm = mtc.create_run_load(from_cfg="dynamics.yml", perform_sweep=True)

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Opinionet"]
        cfg = uni["cfg"]["Opinionet"]
        initial_opinions = data["nw"]["opinion"].isel(time=0)
        final_opinions = data["nw"]["opinion"].isel(time=-1)
        num_vertices = cfg["network"]["num_vertices"]
        opinion_space = cfg["opinion_space"]
        susceptibility = cfg["susceptibility"]
        tolerance = cfg["tolerance"]

        # Check that the total number of users matches the number of vertices
        assert len(final_opinions.coords["vertex_idx"].data) == num_vertices

        # Check that continuous opinions are all within a narrow range
        if (
            "opinion_space" != "discrete"
            and cfg["interaction_function"] != "Deffuant"
        ):

            assert all(
                np.abs(val - final_opinions[0]) < 0.05
                for val in final_opinions
            )

        # Check that discrete opinions remain within the opinion space
        if cfg["opinion_space"]["type"] == "discrete":
            op_space = list(range(cfg["opinion_space"]["num_opinions"]))
            op_vals = list(set(final_opinions.data))

            assert all(op in op_space for op in op_vals)


def test_rewiring():
    """Test the rewiring dynamics"""

    mv, dm = mtc.create_run_load(from_cfg="rewiring.yml", perform_sweep=True)

    # Helper function: gets the sum of neighboring opinion differences
    def get_sum_of_op_diff(nw, opinions):
        sum_of_neighbours_op_diff = 0
        for v in nx.nodes(nw):
            opinion_v = opinions[v]
            for nb in nx.neighbors(nw_init, v):
                opinion_nb = opinions[nb]
                sum_of_neighbours_op_diff += abs(opinion_v - opinion_nb)

        return sum_of_neighbours_op_diff

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Opinionet"]
        initial_opinions = data["nw"]["opinion"].isel(time=0).data
        final_opinions = data["nw"]["opinion"].isel(time=-1).data
        edges_initial = data["nw"]["_edges"].isel(time=0).data
        edges_final = data["nw"]["_edges"].isel(time=-1).data

        # Check number of edges is unchanged
        assert (
            all(len(edges_initial[i]) == len(edges_final[i]))
            for i in range(len(edges_final))
        )

        # Check edges have changed
        assert (
            all(edges_initial[i] != edges_final[i])
            for i in range(len(edges_final))
        )

        # Get intial and final graphs
        nw_init = GraphPlot.create_graph_from_group(data["nw"], at_time_idx=0)
        nw_final = GraphPlot.create_graph_from_group(
            data["nw"], at_time_idx=-1
        )

        # Get the total sums of opinion differences between neighboring vertices
        sum_of_op_diff_init = get_sum_of_op_diff(nw_init, initial_opinions)
        sum_of_op_diff_final = get_sum_of_op_diff(nw_final, final_opinions)

        # Check that, statistially, rewiring leads to differences in neighbors'
        # opinions decreasing
        assert sum_of_op_diff_init > sum_of_op_diff_final
