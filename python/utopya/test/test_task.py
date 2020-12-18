"""Test the Task class implementation"""

import os
import queue
import io
import logging
from time import sleep
from functools import partial

import numpy as np
import pytest

from utopya.task import (Task, WorkerTask, TaskList,
                         PopenMPProcess, MPProcessTask,
                         enqueue_lines, parse_yaml_dict, SIGMAP)

# Set logger level to debug to see what's going on
logging.getLogger("utopya.task").setLevel(1)  # show everything


# Fixtures ----------------------------------------------------------------

@pytest.fixture
def tasks() -> TaskList:
    """Returns a TaskList filled with tasks"""
    tasks = TaskList()
    for name, priority in enumerate(np.random.random(size=50)):
        tasks.append(Task(name=name, priority=priority))

    return tasks

@pytest.fixture
def workertasks() -> TaskList:
    """Returns a TaskList filled with WorkerTasks"""
    tasks = TaskList()
    for name, priority in enumerate(np.random.random(size=50)):
        tasks.append(WorkerTask(name=name, priority=priority,
                                worker_kwargs=dict(args=('echo', '$?'))))

    return tasks

# Task tests ------------------------------------------------------------------

def test_task_init():
    """Test task initialization"""
    Task()  # will get a UID as name
    Task(name=0)
    Task(name=1, priority=1000)
    Task(name=2, priority=1000.01)

    # Test that the task name cannot be changed
    with pytest.raises(AttributeError):
        t = Task(name="foo")
        t.name = "bar"

def test_task_progress():
    # No progess after init
    assert Task().progress == 0.

    # Bad progress function
    t = Task(progress_func=lambda _: 1.01)
    with pytest.raises(ValueError, match="value outside"):
        t.progress

    # Valid progress function
    t._progress_func = lambda _: 0.5
    assert t.progress == 0.5


def test_task_sorting(tasks):
    """Tests whether different task objects are sortable"""
    # Put into a normal list, as the TaskList does not support sorting
    tasks = list(tasks)

    # Sort it, then do some checks
    tasks.sort()

    t1 = tasks[0]
    t2 = tasks[1]

    assert (t1 <= t2) is True
    assert (t1 == t2) is False
    assert (t1 == t1) is True

def test_task_properties(tasks):
    """Test properties and magic methods"""
    _ = [str(t) for t in tasks]

    # Check if the name is given
    task = Task(name="foo")
    assert task.name == task._name == "foo"

    # Check if the UID is returned if no name was given
    task = Task()
    assert task.name == str(task.uid)

    # Check if the default priority is given
    assert task.priority == np.inf

# WorkerTask tests ------------------------------------------------------------

def test_workertask_init():
    """Tests the WorkerTask class"""
    WorkerTask(name=0, worker_kwargs=dict(foo="bar"))

    with pytest.warns(UserWarning, match="`worker_kwargs` given but also"):
        WorkerTask(name=0, setup_kwargs=dict(foo="bar"),
                   worker_kwargs=dict(foo="bar"))

    with pytest.raises(ValueError, match="Need either argument `setup_func`"):
        WorkerTask(name=0)

def test_workertask_magic_methods(workertasks):
    """Test magic methods"""
    _ = [str(t) for t in workertasks]

def test_workertask_invalid_args():
    """Test for invalid argument to the WorkerTask"""
    # It should not be possible to spawn a worker with non-tuple arguments
    t = WorkerTask(name=0, worker_kwargs=dict(args="python -c 'hello hello'"))

    with pytest.raises(TypeError):
        t.spawn_worker()

    # Wanting to save streams but not giving a path should raise an error
    t = WorkerTask(name=1, worker_kwargs=dict(args=("python", "--version"),
                                              save_streams=True))

    with pytest.raises(ValueError, match="Was told to `save_streams` but"):
        t.spawn_worker()

    # Test args pointing to a missing path
    t = WorkerTask(name=2,
                   worker_kwargs=dict(args=("/i/do/not/exist", "--arg1")))
    with pytest.raises(FileNotFoundError,
                       match="No executable found for task '2'!"):
        t.spawn_worker()

def test_workertask_streams(tmpdir):
    """Tests the read_ and save_streams methods of the WorkerTask"""
    save_path = tmpdir.join("out.log")

    t = WorkerTask(name="stream_test",
                   worker_kwargs=dict(args=("echo", "foo\nbar\nbaz"),
                                      read_stdout=True,
                                      stdout_parser="yaml_dict",
                                      save_streams=True,
                                      save_streams_to=str(save_path)))

    # There are no streams yet, so trying to save streams now should not
    # generate a file at save_path
    t.save_streams()
    assert not save_path.isfile()

    # Now spawn the worker
    t.spawn_worker()

    # Wait until done
    while t.worker.poll() is None:
        # Delay the while loop
        sleep(0.05)

    # Add additional sleep to avoid any form of race condition
    sleep(0.2)

    # Read a single line
    t.read_streams(max_num_reads=1)
    print(t.streams['out']['log'])
    assert len(t.streams['out']['log']) == 1

    # Read all the remaining stream content
    t.read_streams(max_num_reads=-1)
    print(t.streams['out']['log'])
    assert len(t.streams['out']['log']) == 3

    # Save it
    t.save_streams()
    assert save_path.isfile()

    # Check the content
    with open(save_path) as f:
        lines = [line.strip() for line in f]

    assert len(lines) == 6
    assert lines[0].startswith(
        "Log of 'out' stream of WorkerTask 'stream_test'"
    )
    assert lines[3:] == ["foo", "bar", "baz"]

    # Trying to save the streams again, there should be no more lines available
    t.save_streams()

def test_workertask_streams_stderr(tmpdir):
    """Tests that not only the stdout is read, but also stderr"""
    save_path = tmpdir.join("out.log")

    # Define the arguments tuple and the WorkerTask
    write_to_stderr = ("python", "-c",
                       # Python commands start here; needs double-escaping!
                       'import sys;'
                       'print("start");'
                       'sys.stderr.write("err1\\nerr2\\n");'
                       'sys.stderr.flush();'
                       'print("end")')
    # This should create four lines, though not necessarily in that order

    t = WorkerTask(name="stderr_test",
                   worker_kwargs=dict(args=write_to_stderr,
                                      read_stdout=True,
                                      stdout_parser="yaml_dict",
                                      save_streams=True,
                                      save_streams_to=str(save_path)))


    # Now spawn the worker and wait until done
    t.spawn_worker()

    while t.worker.poll() is None:
        sleep(0.05)

    # Add additional sleep to avoid any form of race condition
    sleep(0.2)

    # Read the stream's content until empty
    t.read_streams(max_num_reads=-1)
    out_log = t.streams['out']['log']
    print(out_log)

    # Check that the content is as expected
    assert len(out_log) == 4
    assert "err1" in out_log
    assert "err2" in out_log
    assert "start" in out_log
    assert "end" in out_log

# TaskList tests --------------------------------------------------------------

def test_tasklist(tasks):
    """Tests the TaskList features"""
    # Add some tasks
    for _ in range(3):
        tasks.append(Task(name=len(tasks)))

    # This should not work: the tasks already exist
    with pytest.raises(ValueError):
        tasks.append(tasks[0])

    # Or it was not even a task
    with pytest.raises(TypeError):
        tasks.append(("foo", "bar"))

    # Check the __contains__ method
    assert ("foo",) not in tasks
    assert all([task in tasks for task in tasks])

    # Check that locking is possible and prevents appending
    tasks.lock()
    with pytest.raises(RuntimeError, match="TaskList locked!"):
        tasks.append(Task(name="poor little task that won't get added"))

# Additional functions in tasks module ----------------------------------------

def test_enqueue_lines():
    """Test the enqueue lines method"""
    # Create a queue to add to and check the output in
    q = queue.Queue()

    # Test a working example
    enqueue_lines(queue=q,
                  stream=io.StringIO("hello"))
    assert q.get_nowait() == ("hello", None)
    assert q.empty()

    # Test passing a custom parse function
    enqueue_lines(queue=q,
                  stream=io.StringIO("hello yourself"),
                  parse_func=lambda s: s + "!")
    assert q.get_nowait() == ("hello yourself", "hello yourself!")
    assert q.empty()

    # integer parsable should remain strings
    enqueue_lines(queue=q,
                  stream=io.StringIO("1"))
    assert q.get_nowait() == ("1", None)
    assert q.empty()

    # how about multiple lines?
    enqueue_lines(queue=q,
                  stream=io.StringIO("hello\nworld\n!"))
    assert q.get_nowait() == ("hello", None)
    assert q.get_nowait() == ("world", None)
    assert q.get_nowait() == ("!", None)
    assert q.empty()

def test_enqueue_lines_parse_yaml():
    """Test the yaml parsing method for enqueue_lines"""
    # Create a queue to add to and check the output in
    q = queue.Queue()

    enqueue_yaml = partial(enqueue_lines, parse_func=parse_yaml_dict)

    # Working example
    enqueue_yaml(queue=q,
                 stream=io.StringIO("!!map {\"foo\": 123}"))
    assert q.get_nowait()[1] == dict(foo=123)
    assert q.empty()

    # Something not matching the start string should yield no parsed object
    enqueue_yaml(queue=q,
                 stream=io.StringIO("1"))
    assert q.get_nowait()[1] is None
    assert q.empty()

    # Invalid syntax should also return None instead of a parsed object
    enqueue_yaml(queue=q,
                 stream=io.StringIO("!!map {\"foo\": 123, !!invalid}"))
    assert q.get_nowait()[1] is None
    assert q.empty()

    # Line break within the YAML string will lead to parsing failure
    enqueue_yaml(queue=q,
                 stream=io.StringIO("!!map {\"foo\": \n 123}"))
    assert q.get_nowait()[1] is None
    assert q.get_nowait()[1] is None
    assert q.empty()


# -----------------------------------------------------------------------------
# -- multiprocessing support --------------------------------------------------

# Some test callables
# These need to be module-level objects in order to allow them to be pickled.
def pmp_test_target(N: int = 5, *args):
    import sys
    from time import sleep

    print("Got args:", args)
    print(f"Now sleeping {N} times ...")
    for i in range(N):
        print(f"#{i}")
        sleep(.1)

    # Some YAML-parsable line
    print("!!map {\"foo\": 123}")

    # Some error output
    sys.stderr.write("not really an error, but written to stderr\n")

    print("All done! :)")


def test_MPProcessTask():
    """Tests the MPProcessTask specialization"""
    t = MPProcessTask(name=0, worker_kwargs=dict(args=(pmp_test_target,),
                                                 read_stdout=True))

    # String representation is updated accordingly
    assert "MPProcessTask" in str(t)

    # Let it spawn a worker
    t.spawn_worker()

    # ... and check that the interface works as expected
    assert t.worker_status is None  # == still running
    assert isinstance(t.worker, PopenMPProcess)
    assert t.worker_pid > 0
    assert t.worker_pid == t.worker.pid

    # Can read streams ... which will be empty at this point
    t.read_streams()
    assert "\n".join(t.streams['out']['log_raw']) == ""

    # Wait until the process finished
    while t.worker_status is None:
        sleep(.1)
    assert t.worker_status == 0

    # Stream reader threads also have shut down (might need some time)
    sleep(.1)
    assert not t.streams['out']['thread'].is_alive()

    # Manually get the output from the streams and the queue
    s = t.worker.stdout
    print("stdout object", s)
    print("from stream object:\n" + "".join(s.readlines()))
    print("\nfrom file:\n" + "".join(open(s.name).readlines()))

    q = t.streams['out']['queue']
    assert q.empty()

    # Now should have streams available and don't need to call read_streams,
    # because the worker has already finished
    print("out stream metadata:", t.streams['out'])
    captured_stdout = "\n".join(t.streams['out']['log_raw'])
    assert "Now sleeping 5 times ..." in captured_stdout
    assert "All done! :)" in captured_stdout

    # stderr was forwarded
    assert "not really an error, but written to stderr" in captured_stdout

    # By default, won't have any outstream objects
    assert not t.outstream_objs
    assert "!!map" in captured_stdout

    # But if reading with a different parser, will have:
    t = MPProcessTask(name=1,
                      worker_kwargs=dict(args=(pmp_test_target,),
                                         read_stdout=True,
                                         stdout_parser="yaml_dict"))
    t.spawn_worker()
    while t.worker_status is None:
        sleep(.1)
    assert t.worker_status == 0
    print("out stream metadata:", t.streams)
    assert t.outstream_objs == [dict(foo=123)]


# .. PopenMPProcess wrapper ...................................................

PMP = PopenMPProcess

def test_PopenMPProcess_basics():
    """Tests the PopenMPProcess wrapper around multiprocessing.Process"""
    # Basic initialization, which directly starts the process ...
    proc = PMP((pmp_test_target, 5, ("foo", "bar"),))

    # The underlying multiprocess.Process is alive now
    assert proc._proc.is_alive()

    # Test the subprocess.Popen interface of the wrapper
    assert proc.poll() is None
    assert proc.returncode is None
    assert proc.pid > 0
    assert proc.stdin is None
    assert proc.stdout is None
    assert proc.stderr is None
    assert proc.args == (pmp_test_target, 5, ("foo", "bar"),)

    # Parts of the interface are NOT implemented
    with pytest.raises(NotImplementedError):
        proc.wait()

    with pytest.raises(NotImplementedError):
        proc.communicate()

    # It should still be running now
    assert proc.poll() is None

    # Manually wait for it to finish
    while proc.poll() != 0:
        sleep(.1)

    assert proc.poll() == 0 == proc.returncode

def test_PopenMPProcess_signal():
    """Test signalling for the PopenMPProcess"""

    args = (pmp_test_target, 50, ("foo", "bar"),)
    proc = PMP(args)
    assert proc.poll() is None

    # Can't send a custom signal
    with pytest.raises(NotImplementedError):
        proc.send_signal(123)

    # Give it some time to start up, otherwise SIGTERM might fail
    sleep(1.)

    # But can terminate it
    proc.send_signal(SIGMAP['SIGTERM'])
    sleep(.1)
    assert proc.poll() == -SIGMAP['SIGTERM'] == proc.returncode

    # Can attempt to kill it now, but has no effect
    proc.send_signal(SIGMAP['SIGKILL'])
    assert proc.returncode == -SIGMAP['SIGTERM']

    # Another one, now with SIGKILL
    proc = PMP(args)
    assert proc.poll() is None
    sleep(1.)

    proc.kill()
    sleep(.1)
    assert proc.poll() == -SIGMAP['SIGKILL'] == proc.returncode


def test_PopenMPProcess_streams():
    """Test stream handling of the PopenMPProcess"""
    import subprocess
    PMP = PopenMPProcess

    # Start process with separate streams for stdout and stderr
    args = (pmp_test_target, 3, ("foo", "bar"))
    proc = PMP(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert proc.poll() is None

    # Have stream objects associated now ... still empty though
    assert proc.stdout is not None
    assert proc.stdout.read() == ""

    assert proc.stderr is not None
    assert proc.stderr.read() == ""

    # Wait for it to finish
    while proc.poll() != 0:
        sleep(.1)
    captured_stdout = proc.stdout.read()
    captured_stderr = proc.stderr.read()
    assert captured_stdout != ""
    assert captured_stderr != ""
    assert captured_stderr != captured_stdout

    assert "Now sleeping 3 times ..." in captured_stdout
    assert "All done! :)" in captured_stdout

    assert "written to stderr" in captured_stderr


    # Try out all other combinations
    test_combinations = (
        # (stdout, stderr),
        (None, None),
        (subprocess.STDOUT, subprocess.PIPE),  # two separate files
        (subprocess.PIPE, subprocess.PIPE),    # two separate files
        (subprocess.DEVNULL, subprocess.PIPE), # stdout to /dev/null, one file
        (subprocess.PIPE, subprocess.DEVNULL), # stderr to /dev/null, one file
        (subprocess.DEVNULL, subprocess.DEVNULL), # all to /dev/null
    )

    for stdout, stderr in test_combinations:
        print(f"Testing with stdout={stdout} and stderr={stderr} ...")

        proc = PMP(args, stdout=stdout, stderr=stderr)
        assert proc.poll() is None
        while proc.poll() != 0:
            sleep(.1)

        if stdout is None:
            assert proc.stdout is None
        elif stdout is subprocess.DEVNULL:
            assert proc.stdout.name == os.devnull
        else:
            captured_stdout = proc.stdout.read()

        if stderr is None:
            assert proc.stderr is None
        elif stderr is subprocess.DEVNULL:
            assert proc.stderr.name == os.devnull
        else:
            captured_stderr = proc.stderr.read()

    # stdin is not supported
    with pytest.raises(NotImplementedError):
        PMP(args, stdin="foo")
