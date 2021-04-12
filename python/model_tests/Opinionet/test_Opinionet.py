"""Tests of the output of the Opinionet model"""
import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for Opinionet
mtc = ModelTest("Opinionet", test_file=__file__)

# Tests -----------------------------------------------------------------------

def test_static():
    """Test the initialization of the model and the case of static opinions"""

    # Create a Multiverse using the default model configuration
    mv, dm = mtc.create_run_load(from_cfg="static.yml",
                                 perform_sweep=True)
    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Opinionet']
        cfg = uni['cfg']['Opinionet']
        initial_opinions = data['nw']['opinion'].isel(time=0)
        final_opinions = data['nw']['opinion'].isel(time=-1)

        # Check that opinions don't change due to susceptibility = 0
        for init, final in zip(initial_opinions, final_opinions):
             assert init == final

        # Check that the total number of vertices is correct
        num_vertices = cfg['network']['num_vertices']
        users = data['nw']['_vertices'].data
        assert len(users) == num_vertices

        # Check that edge weight data was written for directed graphs
        # Check that edge weights don't change due to rewiring = false
        # Check that edge weights are normalised probabilities
        if (cfg['network']['directed'] == True):
            initial_weights = data['nw']['edge_weights'].isel(time=0)
            final_weights = data['nw']['edge_weights'].isel(time=-1)

            for init, final in zip(initial_weights, final_weights):
                 assert init == final

            edge_weights = np.append(initial_weights, final_weights)

            for weight in edge_weights:
                assert (0<=weight<=1 and not np.isnan(weight))

            assert (np.sum(edge_weights)==2*num_vertices)

        # For continuous opinion space: check opinions are uniformly distributed
        # over opinion space.
        if (cfg['opinion_space']['type'] == 'continuous'):
            op_space = cfg['opinion_space']['interval']
            op_space_center = 0.5*(op_space[1]+op_space[0])
            assert (op_space[0]<=np.min(initial_opinions))
            assert (np.max(initial_opinions)<=op_space[1])
            assert (op_space_center-0.05<np.mean(initial_opinions)<op_space_center+0.05)

        # For discrete opinion space: check all opinions are integer values in
        # the opinion space
        elif (cfg['opinion_space']['type'] == 'discrete'):
            op_space = list(range(cfg['opinion_space']['num_opinions']))
            op_vals = list(set(initial_opinions.data))
            assert(op_space == op_vals)

def test_single_revision():
    """Test a single revision step"""
    mv, dm = mtc.create_run_load(from_cfg="single_revision.yml",
                                 perform_sweep=True)

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Opinionet']
        cfg = uni['cfg']['Opinionet']
        initial_opinions = data['nw']['opinion'].isel(time=0)
        final_opinions = data['nw']['opinion'].isel(time=-1)
        opinion_space = cfg['opinion_space']
        susceptibility = cfg['susceptibility']
        tolerance = cfg['tolerance']

        # Check opinion did not leave opinion space
        # Check that opinion did not change by more than tolerance * susceptibility
        # except for discrete Deffuant, where it should not change by more than
        # the tolerance
        if (opinion_space == "continuous"):
            op_space = cfg['opinion_space']['interval']
            assert (op_space[0]<=np.min(initial_opinions))
            assert (np.max(initial_opinions)<=op_space[1])
            for init, final in zip(initial_opinions, final_opinions):
                assert abs(init - final) <= susceptibility * tolerance

        elif (opinion_space == 'discrete'):
            op_space = list(range(cfg['opinion_space']['num_opinions']))
            op_vals = list(set(final_opinions.data))
            assert(op_space == op_vals)
            if (cfg['interaction_function'] != 'Deffuant'):
                for init, final in zip(initial_opinions, final_opinions):
                    assert abs(init - final) <= susceptibility * tolerance
            else:
                for init, final in zip(initial_opinions, final_opinions):
                    assert abs(init - final) <= tolerance

# def test_dynamics():
#     """Test the general dynamics"""
#     mv, dm = mtc.create_run_load(from_cfg="dynamics.yml", perform_sweep=True)
#
#     for uni_no, uni in dm['multiverse'].items():
#         data = uni['data']['Opinionet']
#         cfg = uni['cfg']['Opinionet']
#         initial_opinions = data['nw']['opinion'].isel(time=0)
#         final_opinions = data['nw']['opinion'].isel(time=-1)
#         opinion_space = cfg['opinion_space']
#         susceptibility = cfg['susceptibility']
#         tolerance = cfg['tolerance']
#
#         # Check that the total number of users matches the number of vertices
#         assert final_users.sum(dim='vertex_idx').values == 20
#
#         # Check that the opinions are all within a narrow range
#         assert all(np.abs(val - final_opinion_u[0]) < 0.05
#                     for val in final_opinion_u)
#
# def test_rewiring():
