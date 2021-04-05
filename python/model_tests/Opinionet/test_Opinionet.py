"""Tests of the output of the Opinionet model"""
import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for Opinionet
mtc = ModelTest("Opinionet", test_file=__file__)

# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_static():
    """Test the initialization of the model and the case of static opinions"""

    # Create a Multiverse using the default model configuration
    mv, dm = mtc.create_run_load(from_cfg="static.yml",
                                 perform_sweep=False)
    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

#     data = dm['multiverse'][0]['data']['Opinionet']
#     initial_opinion_u = data['nw_users']['opinion_u'].isel(time=0)
#     initial_opinion_m = data['nw_media']['opinion_m'].isel(time=0)
#     final_opinion_u = data['nw_users']['opinion_u'].isel(time=-1)
#     final_opinion_m = data['nw_media']['opinion_m'].isel(time=-1)
#     initial_users = data['nw_media']['users'].isel(time=0)
#     final_users = data['nw_media']['users'].isel(time=-1)
#
#     # Check that opinions are uniformly distributed over [0, 1]
#     assert (0.45 < np.mean(initial_opinion_u) < 0.55)
#     assert (0.45 < np.mean(initial_opinion_m) < 0.55)
#
#     # Check that opinions don't change due to persuasiveness=0
#     for init, final in zip(initial_opinion_u, final_opinion_u):
#         assert init == final
#     for init, final in zip(initial_opinion_m, final_opinion_m):
#         assert init == final
#
#     # Check that the total number of users matches the number of vertices
#     assert initial_users.sum(dim='vertex_idx').values == 1000
#     assert final_users.sum(dim='vertex_idx').values == 1000
#
#
# def test_single_revision():
#     """Test a single revision step"""
#     mv, dm = mtc.create_run_load(from_cfg="single_revision.yml",
#                                  perform_sweep=False)
#
#     data = dm['multiverse'][0]['data']['Opinionet']
#     initial_opinion_u = data['nw_users']['opinion_u'].isel(time=0)
#     initial_opinion_m = data['nw_media']['opinion_m'].isel(time=0)
#     final_opinion_u = data['nw_users']['opinion_u'].isel(time=-1)
#     final_opinion_m = data['nw_media']['opinion_m'].isel(time=-1)
#
#     # Check that opinion doesn't change by more than tolerance * persuasiveness
#     for init, final in zip(initial_opinion_u, final_opinion_u):
#         assert abs(init - final) < 0.2
#     for init, final in zip(initial_opinion_m, final_opinion_m):
#         assert abs(init - final) < 0.2
#
#
# def test_dynamics():
#     """Test the general dynamics"""
#     mv, dm = mtc.create_run_load(from_cfg="dynamics.yml", perform_sweep=False)
#
#     data = dm['multiverse'][0]['data']['Opinionet']
#     final_opinion_u = data['nw_users']['opinion_u'].isel(time=-1)
#     final_users = data['nw_media']['users'].isel(time=-1)
#
#     # Check that the total number of users matches the number of vertices
#     assert final_users.sum(dim='vertex_idx').values == 20
#
#     # Check that the opinions are all within a narrow range
#     assert all(np.abs(val - final_opinion_u[0]) < 0.05
#                 for val in final_opinion_u)
