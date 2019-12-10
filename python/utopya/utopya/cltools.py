"""Methods needed to implement the utopia command line interface"""

import os
import re
import glob
import logging
from typing import Callable, Dict, Sequence, Tuple
from pkg_resources import resource_filename

from .multiverse import Multiverse
from .tools import recursive_update, add_item
from .cfg import load_from_cfg_dir, write_to_cfg_dir
from .model_registry import get_info_bundle as _get_info_bundle
from . import MODELS as _MODELS

# Local constants
log = logging.getLogger(__name__)

USER_CFG_HEADER_PATH = resource_filename('utopya', 'cfg/user_cfg_header.yml')
BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')

# -----------------------------------------------------------------------------

def add_from_kv_pairs(*pairs, add_to: dict,
                      attempt_conversion: bool=True,
                      allow_eval: bool=False,
                      allow_deletion: bool=True) -> None:
    """Parses the key=value pairs and adds them to the given dict.

    Note that this happens directly on the object, i.e. making use of the
    mutability of the given dict. This function has no return value!
    
    Args:
        *pairs: Sequence of key=value strings
        add_to (dict): The dict to add the pairs to
        attempt_conversion (bool, optional): Whether to attempt converting the
            strings to bool, float, int types
        allow_eval (bool, optional): Whether to try calling eval() on the
            value strings during conversion
        allow_deletion (bool, optional): If set, can pass DELETE string to
            a key to remove the corresponding entry.
    """
    # Object to symbolise deletion
    class _DEL: pass
    DEL = _DEL()

    def conversions(val):
        # Boolean
        if val.lower() in ["true", "false"]:
            return bool(val.lower() == "true")
        
        # None
        if val.lower() in ["null"]:
            return None

        # Floating point number (requiring '.' being present)
        if re.match(r'^[-+]?[0-9]*\.[0-9]*([eE][-+]?[0-9]+)?$', val):
            try:
                return float(val)
            except:
                pass

        # Integer
        if re.match(r'^[-+]?[0-9]+$', val):
            try:
                return int(val)
            except: # very unlike to be reached; regex is quite restrictive
                pass

        # Deletion placeholder
        if val == "DELETE":
            return DEL

        # Last resort, if activated: eval
        if allow_eval:
            try:
                return eval(val)
            except:
                pass

        # Just return the string
        return val

    log.debug("Adding entries from key-value pairs ...")

    # Go over all pairs and add them to the given base dict
    for kv in pairs:
        # Split key and value
        key, val = kv.split("=")

        # Process the key
        key_sequence = key.split(".")
        traverse_keys, last_key = key_sequence[:-1], key_sequence[-1]

        # Set temporary variable to root dict
        d = add_to

        # Traverse through the key sequence, if available
        for _key in traverse_keys:
            # Check if a new entry is needed
            if _key not in d:
                d[_key] = dict()

            # Select the new entry
            d = d[_key]

        # Attempt conversion
        if attempt_conversion:
            val = conversions(val)

        # In all cases but that where the value is the DEL object, write:
        if val is not DEL:
            d[last_key] = val
            continue

        # Otherwise: need to check whether deletion is allowed and the entry
        # is present ...
        if not allow_deletion:
            raise ValueError("Attempted deletion of value for key '{}', but "
                             "deletion is not allowed.".format(key))

        if last_key not in d:
            continue
        del d[last_key]

    # No need to return the base dict as it is a mutable!
    log.debug("Added %d entries from key-value pairs.", len(pairs))


def register_models(args, *, registry):
    """Handles registration of multiple models given argparse args"""
    # If there is project info to be updated, do so
    project_info = None
    if args.update_project_info:
        project_info = register_project(args, arg_prefix='project_')

    # The dict to hold all model info bundle arguments
    specs = dict()

    if not args.separator:
        # Will only register a single model.
        # Gather all the path-related arguments
        # TODO
        raise NotImplementedError("Registering a single model is currently "
                                  "not possible via the CLI!")
        # paths = dict()
        # specs[args.model_name] = dict(paths=paths)

    else:
        # Got separator for lists of model names, binary paths, and source dirs
        log.debug("Splitting given model registration arguments by '%s' ...",
                  args.separator)

        model_names = args.model_name.split(args.separator)
        bin_paths = args.bin_path.split(args.separator)
        src_dirs = args.src_dir.split(args.separator)

        if not (len(model_names) == len(bin_paths) == len(src_dirs)):
            raise ValueError("Mismatch of sequence lengths during batch model "
                             "registration! The model_name, bin_path, and "
                             "src_dir lists should all be of equal length "
                             "after having been split by separator '{}', but "
                             "were: {}, {}, and {}, respectively."
                             "".format(args.separator, model_names,
                                       bin_paths, src_dirs))
        # TODO Will ignore other path-related arguments! Warn if given.

        # Go over them, create the paths dict, and populate specs dict.
        # If there is project info given, use it to extend path information
        # with the python-related directories. Only do so if they exist.
        for model_name, bin_path, src_dir in zip(model_names,
                                                 bin_paths, src_dirs):
            paths = dict(src_dir=src_dir,
                         binary=bin_path,
                         base_src_dir=args.base_src_dir,
                         base_bin_dir=args.base_bin_dir)

            if project_info:
                for _k in ('python_model_tests_dir', 'python_model_plots_dir'):
                    _path = os.path.join(project_info[_k], model_name)
                    if os.path.isdir(_path):
                        paths[_k] = _path

            specs[model_name] = dict(paths=paths,
                                     project_name=args.project_name)

    log.debug("Received registry parameters for %d model%s.",
              len(specs), "s" if len(specs) != 1 else "")

    # Now, actually register. Here, pass along the common arguments.
    for model_name, bundle_kwargs in specs.items():
        registry.register_model_info(model_name, **bundle_kwargs,
                                     exists_action=args.exists_action,
                                     label=args.label,
                                     overwrite_label=args.overwrite_label)

    log.info("Model registration finished.\n\n%s\n", registry.info_str)


def register_project(args, *, arg_prefix: str='') -> dict:
    """Register or update information of an Utopia project, i.e. a repository
    that implements models.
    
    Args:
        args (TYPE): The CLI arguments object
        arg_prefix (str, optional): The prefix to use when using attribute
            access to these arguments. Useful if the names as defined in the
            CLI are different depending on the invocation
    
    Returns:
        dict: Information on the newly added or updated project
    """
    project_name = getattr(args, arg_prefix + "name")
    log.debug("Adding or updating information for Utopia project '%s' ...",
              project_name)

    project_paths = dict()
    for arg_name in ('base_dir', 'models_dir',
                     'python_model_tests_dir', 'python_model_plots_dir'):
        project_paths[arg_name] = getattr(args, arg_prefix + arg_name)

        if project_paths[arg_name]:
            project_paths[arg_name] = str(project_paths[arg_name])

    # Load existing project information, update it, store back to file
    projects = load_from_cfg_dir('projects')  # empty dict if file is missing
    projects[project_name] = project_paths

    write_to_cfg_dir('projects', projects)
    log.info("Updated information for Utopia project '%s'.", project_name)

    # If python_model_plots_dir is given, update plot modules cfg file
    if project_paths['python_model_plots_dir']:
        log.debug("Additionally updating the python model plots path ...")

        plot_module_paths = load_from_cfg_dir('plot_module_paths')
        model_plots_dir = project_paths['python_model_plots_dir']

        # Remove duplicate paths and instead store it under the project name
        plot_module_paths = {k:v for k,v in plot_module_paths.items()
                             if v != model_plots_dir}
        plot_module_paths[project_name] = model_plots_dir

        write_to_cfg_dir('plot_module_paths', plot_module_paths)
        log.info("Updated plot module paths for Utopia project '%s'.",
                 project_name)

    # Return the project information
    return projects[project_name]


def deploy_user_cfg(user_cfg_path: str=Multiverse.USER_CFG_SEARCH_PATH
                    ) -> None:
    """Deploys a copy of the full config to the specified location (usually
    the user config search path of the Multiverse class)
    
    Instead of just copying the full config, it is written line by line,
    commenting out lines that are not already commented out, and changing the
    header.
    
    Args:
        user_cfg_path (str, optional): The path the file is expected at. Is an
            argument in order to make testing easier.
    
    Returns:
        None
    """
    # Check if a user config already exists
    if os.path.isfile(user_cfg_path):
        # There already is one. Ask if this should be overwritten...
        print("A config file already exists at " + str(user_cfg_path))
        if input("Replace? [y, N]  ").lower() in ['yes', 'y']:
            # Delete the file
            os.remove(user_cfg_path)
            print("")

        else:
            # Abort here
            print("Not deploying user config.")
            return
    
    # At this point, can assume that it is desired to write the file and there
    # is no other file there
    # Make sure that the folder exists
    os.makedirs(os.path.dirname(user_cfg_path), exist_ok=True)

    # Create a file at the given location
    with open(user_cfg_path, 'x') as ucfg:
        # Write header section, from user config header file
        with open(USER_CFG_HEADER_PATH, 'r') as ucfg_header:
            ucfg.write(ucfg_header.read())

        # Now go over the full config and write the content, commenting out
        # the lines that are not already commented out
        with open(BASE_CFG_PATH, 'r') as bcfg:
            past_prefix = False

            for line in bcfg:
                # Look for "---" to find out when the header section ended
                if line == "---\n":
                    past_prefix = True
                    continue

                # Write only if past the prefix
                if not past_prefix:
                    continue

                # Check if the line in the target (user) config needs to be
                # commented out or not
                if line.strip().startswith("#") or line.strip() == "":
                    # Is a comment or empty line -> just write it
                    ucfg.write(line)

                else:
                    # There is an entry on this line -> comment out before the
                    # first character (looks cleaner)
                    spaces = " " * (len(line.rstrip()) - len(line.strip()))
                    ucfg.write(spaces + "# " + line[len(spaces):])
        # Done

    print("Deployed user config to: {}\n\nAll entries are commented out; "
          "open the file to edit your configuration. Note that it is wise to "
          "only enable those entries that you absolutely _need_ to set."
          .format(user_cfg_path))


def copy_model_files(*, model_name: str,
                     new_name: str=None, target_project: str=None,
                     add_to_cmakelists: bool=True,
                     skip_exts: Sequence[str]=None,
                     use_prompts: bool=True,
                     dry_run: bool=False) -> None:
    """A helper function to conveniently copy model-related files, rename them,
    and adjust their content to the new name as well.
    
    Args:
        model_name (str): The name of the model to copy
        new_name (str, optional): The new name of the model. This may not
            conflict with any already existing model name in the model
            registry.
        target_project (str, optional): The name of the project to copy the
            model to. It needs to be a registered Utopia project.
        add_to_cmakelists (bool, optional): Whether to add the new model to the
            corresponding CMakeLists.txt file.
        use_prompts (bool, optional): Whether to interactively prompt for
            confirmation or missing arguments.
        dry_run (bool, optional): If given, no write or copy operations will be
            carried out.
    
    Raises:
        ValueError: Upon bad arguments
    
    Returns:
        None
    """
    def apply_replacements(s, *replacements: Sequence[Tuple[str, str]]) -> str:
        """Applies multiple replacements onto the given string"""
        for replacement in replacements:
            s = s.replace(*replacement)
        return s

    def create_file_map(*, source_dir: str, target_dir: str,
                        abs_file_map: dict,
                        replacements: Sequence[Tuple[str, str]],
                        skip_exts: Sequence[str]=None,
                        glob_args: Sequence[str]=("**",)) -> dict:
        """Given a file list with absolute paths, aggregates the file path
        changes into ``abs_file_map`` and gathers the relative file path
        changes into the returned dict.

        The file name is changed according to the specified replacements.
        
        Args:
            source_dir (str): The source directory to look for files in using
                glob and the ``glob_args``. Note that directories are not
                matched.
            target_dir (str): The target directory of the renamed files
            abs_file_map (dict): The mutable file map that the absolute file
                path changes are aggregated in.
            replacements (Sequence[Tuple[str, str]]): The replacement
                specifications, applied to the relative paths.
            glob_args (Sequence[str], optional): The glob arguments to match
                files within the source directory. By default, this matches
                all files, also down the source directory tree. The glob
                ``recursive`` option is enabled.
        
        Returns:
            dict: The file map relative to source and target dir.
        """
        files = glob.glob(os.path.join(source_dir, *glob_args),
                          recursive=True)
        rel_file_map = dict()

        for fpath in files:
            if os.path.isdir(fpath) or not os.path.exists(fpath):
                continue

            if skip_exts and os.path.splitext(fpath)[1] in skip_exts:
                continue

            rel_fpath = os.path.relpath(fpath, start=source_dir)
            new_rel_fpath = apply_replacements(rel_fpath, *replacements)

            abs_file_map[fpath] = os.path.join(target_dir, new_rel_fpath)
            rel_file_map[rel_fpath] = new_rel_fpath

        return rel_file_map

    def print_file_map(*, file_map: dict, source_dir: str, target_dir: str,
                       label: str):
        """Prints a human-readable version of the given (relative) file map
        which copies from the source directory tree to the target directory
        tree.
        """
        max_key_len = min(max([len(k) for k in file_map]), 32)
        files = ["\t{:{l:d}s}  ->  {:s}".format(k, v, l=max_key_len)
                 for k, v in file_map.items()]

        print("\nThe following {num:d} {label:s} files from\n\t{from_dir:}\n"
              "will be copied to\n\t{to_dir:}\nusing the following new file "
              "names:\n{files:}"
              "".format(num=len(file_map), label=label,
                        from_dir=source_dir, to_dir=target_dir,
                        files="\n".join(files)))

    def add_model_to_cmakelists(*, fpath: str, new_name: str, write: bool):
        """Adds the relevant add_subdirectory command to the CMakeLists file
        at the specified path.
        
        Assumes an ascending alphabetical list of add_subdirectory commands
        and adds the new command at a suitable place.
        
        Args:
            fpath (str): The absolute path of the CMakeLists.txt file
            new_name (str): The new model name to add to it
            write (bool): If false, will not write.
        
        Raises:
            ValueError: On missing ``add_subdirectory`` command in the given
                file. In this case, the line has to be added manually.
        """
        # Read the file
        with open(fpath, 'r') as f:
            lines = f.readlines()

        # Find the line to add the add_subdirectory command at
        insert_idx = None
        for i, line in enumerate(lines):
            if not line.startswith("add_subdirectory"):
                continue

            insert_idx = i
            _model = line[len("add_subdirectory("):-2]
            if _model.lower() > new_name.lower():
                break
        else:
            # Did not break. Insert behind the last add_subdirectory command
            insert_idx += 1

        if insert_idx is None:
            raise ValueError("Found no add_subdirectory commands and thus do "
                             "not know where to insert the command for the "
                             "new model directory; please do it manually in "
                             "the following file:  {}".format(fpath))

        lines.insert(insert_idx if insert_idx
                                else last_add_subdir_idx + 1,
                                "add_subdirectory({})\n".format(new_name))

        if write:
            with open(fpath, 'w') as f:
                f.writelines(lines)
            
            print("Subdirectory for model '{}' added to\n\t{}"
                  "".format(new_name, fpath))

        else:
            print("Not writing. Preview of how the new\n\t{}\nfile _would_ "
                  "look like:".format(fpath))
            print("-"*79 + "\n")
            print("".join(lines))
            print("-"*79)

    # Gather information on model, project, and replacements . . . . . . . . .
    # Get the model information
    info_bundle = _get_info_bundle(model_name=model_name)
    print("\nModel selected to copy:     {}  (from project: {})"
          "".format(info_bundle.model_name, info_bundle.project_name))

    # Find out the new name
    if not new_name:
        if not use_prompts:
            raise ValueError("Missing new_name argument!")
        try:
            new_name = input("\nWhat should be the name of the NEW model?  ")
        except KeyboardInterrupt:
            return

    # Check if the name is not already taken, being case-insensitive
    if new_name.lower() in [n.lower() for n in _MODELS.keys()]:
        raise ValueError("A model with name '{}' is already registered! "
                         "Make sure that the name is unique. If you keep "
                         "receiving this error despite no other model with "
                         "this name being implemented, remove the entry from "
                         "the model registry, e.g. via the `utopia models rm` "
                         "CLI command.\n"
                         "Already registered models: {}"
                         "".format(new_name, ", ".join(_MODELS.keys())))

    print("Name of the new model:      {}".format(new_name))

    # Define the replacements
    replacements = [
        (model_name, new_name),
        (model_name.lower(), new_name.lower()),
        (model_name.upper(), new_name.upper()),
    ]

    # Find out the project that the files are copied _to_
    projects = load_from_cfg_dir('projects')

    if not target_project:
        if not use_prompts:
            raise ValueError("Missing target_project argument!")
        try:
            target_project = input("\nWhich Utopia project (available: {}) "
                                   "should the model be copied to?  "
                                   "".format(", ".join(projects)))
        except KeyboardInterrupt:
            return
    print("Utopia project to copy to:  {}".format(target_project))

    project_info = projects.get(target_project)
    if not project_info:
        raise ValueError("No Utopia project with name '{}' is known to the "
                         "frontend. Check the spelling and note that the "
                         "project name is case-sensitive.\n"
                         "Available projects: {}."
                         "".format(target_project, ", ".join(projects)))
    
    # Generate the file maps . . . . . . . . . . . . . . . . . . . . . . . . .
    # The mapping of all files that are to be copied and in which the content
    # is to be replaced. It maps absolute source file paths to absolute target
    # file paths.
    file_map = dict()

    # Relative file maps, created below
    impl_file_map = None
    py_t_file_map = None
    py_p_file_map = None

    # Find out the target directories
    target_models_dir = project_info.get('models_dir')
    target_py_t_dir = project_info.get('python_model_tests_dir')
    target_py_p_dir = project_info.get('python_model_plots_dir')

    # Define the source and target directory paths of the implementation and
    # the python-related files, if the path information is available.
    impl_source_dir = info_bundle.paths['source_dir']
    impl_target_dir = os.path.join(target_models_dir, new_name)
    impl_file_map = create_file_map(source_dir=impl_source_dir,
                                    target_dir=impl_target_dir,
                                    abs_file_map=file_map,
                                    replacements=replacements,
                                    skip_exts=skip_exts)

    if target_py_t_dir and info_bundle.paths.get('python_model_tests_dir'):
        py_t_source_dir = info_bundle.paths['python_model_tests_dir']
        py_t_target_dir = os.path.join(target_py_t_dir, new_name)
        py_t_file_map = create_file_map(source_dir=py_t_source_dir,
                                        target_dir=py_t_target_dir,
                                        abs_file_map=file_map,
                                        replacements=replacements,
                                        skip_exts=skip_exts)
    
    if target_py_p_dir and info_bundle.paths.get('python_model_plots_dir'):
        py_p_source_dir = info_bundle.paths['python_model_plots_dir']
        py_p_target_dir = os.path.join(target_py_p_dir, new_name)
        py_p_file_map = create_file_map(source_dir=py_p_source_dir,
                                        target_dir=py_p_target_dir,
                                        abs_file_map=file_map,
                                        replacements=replacements,
                                        skip_exts=skip_exts)


    # Gathered all information now. . . . . . . . . . . . . . . . . . . . . . .
    # Inform about the file changes and the replacement in them.
    print_file_map(file_map=impl_file_map, label="model implementation",
                   source_dir=impl_source_dir, target_dir=impl_target_dir)

    if py_t_file_map:
        print_file_map(file_map=py_t_file_map, label="python model test",
                       source_dir=py_t_source_dir, target_dir=py_t_target_dir)
    
    if py_p_file_map:
        print_file_map(file_map=py_p_file_map, label="python model plot",
                       source_dir=py_p_source_dir, target_dir=py_p_target_dir)

    max_repl_len = max([len(rs) for rs, _ in replacements])
    repl_info = ["\t'{:{l:d}s}'  ->  '{:s}'".format(*repl, l=max_repl_len)
                 for repl in replacements]
    print("\nInside all of these {:d} files, the following string "
          "replacements will be carried out:\n{}\n"
          "".format(len(file_map), "\n".join(repl_info)))

    # Inform about dry run and ask whether to proceed
    if dry_run:
        print("--- THIS IS A DRY RUN. ---")
        print("Copy and write operations below are not operational.")

    if use_prompts:
        try:
            response = input("\nProceed [y/N]  ")
        except KeyboardInterrupt:
            response = "N"
        if response.lower() not in ('y', 'yes'):
            print("\nNot proceeding ...")
            return

    print("\nNow copying and refactoring ...")

    # Now, the actual copying . . . . . . . . . . . . . . . . . . . . . . . . .
    for i, (src_fpath, target_fpath) in enumerate(file_map.items()):
        print("\nFile {:d}/{:d} ...".format(i+1, len(file_map)))
        print("\t   {:s}\n\t-> {:s}".format(src_fpath, target_fpath))

        try:
            with open(src_fpath, mode='r') as src_file:
                src_lines = src_file.read()

        except Exception as exc:
            print("\tReading FAILED due to {}: {}."
                  "".format(exc.__class__.__name__, str(exc)))
            print("\tIf you want this file copied and refactored, you will "
                  "have to do it manually.")
            continue

        target_lines = apply_replacements(src_lines, *replacements)

        if dry_run:
            continue

        # Create directories and write the file; failing if it already exists
        os.makedirs(os.path.dirname(target_fpath), exist_ok=True)
        with open(target_fpath, mode='x') as target_file:
            target_file.write(target_lines)

    print("\nFinished copying.\n")

    # Prepare for CMakeLists.txt adjustments
    cmakelists_fpath = os.path.abspath(os.path.join(impl_target_dir,
                                                    "../CMakeLists.txt"))
    
    if not add_to_cmakelists:
        print("Not extending CMakeLists.txt automatically.")
        print("Remember to register the new model in the relevant "
              "CMakeLists.txt file at\n\t{}\nand invoke CMake to reconfigure."
              "".format(cmakelists_fpath))
        return

    print("Adding model directory to CMakeLists.txt ...")
    add_model_to_cmakelists(fpath=cmakelists_fpath, new_name=new_name,
                            write=not dry_run)

    # All done now.
    print("\nFinished.")
