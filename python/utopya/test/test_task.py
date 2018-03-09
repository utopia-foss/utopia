import pytest
from utopya.task import test_task2
from utopya import Task


# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Task for initialisation"""
    return dict(model_name="dummy",
                run_cfg_path=RUN_CFG_PATH,
                user_cfg_path=USER_CFG_PATH)


# Initialisation tests --------------------------------------------------------

def test_simple_init(mv_kwargs):
    """Tests whether initialisation works."""
    Task(**mv_kwargs)

def test_invalid_model_name_and_operation(mv_kwargs, tmpdir):
    """Tests for correct behaviour upon invalid model names"""
    mv_local = mv_kwargs

    # Try to change the model name
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_try_change_model_name"))
    instance = Multiverse(**mv_local, update_meta_cfg=local_config)
    with pytest.raises(RuntimeError):
        instance.model_name = "dummy"

    # Try invalid model name  
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_invalid_model_name"))
    mv_local['model_name'] = "invalid_model_RandomShit_bgsbjkbkfvwuRfopiwehGEP"

    with pytest.raises(ValueError):
        Multiverse(**mv_local, update_meta_cfg=local_config)

'''
tasks=[]
for i in range(10):
    tasks.append(Task(i,rd.randint(-10,10)))
    
print(tasks)
print(sorted(tasks))
#'''