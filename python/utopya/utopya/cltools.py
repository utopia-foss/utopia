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
    # The dict to hold all arguments
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

        # Go over them, create the path_args dict, and populate specs dict
        for model_name, bin_path, src_dir in zip(model_names,
                                                 bin_paths, src_dirs):
            paths = dict(src_dir=src_dir,
                         binary=bin_path,
                         base_src_dir=args.base_src_dir,
                         base_bin_dir=args.base_bin_dir)
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

    # If there is to be project info registered, pass the arguments on
    if args.update_project_info:
        register_project(args, arg_prefix='project_')


def register_project(args, *, arg_prefix: str=''):
    """Register or update information of an Utopia project, i.e. a repository
    that implements models.
    """
    project_name = args.project_name
    project_paths = dict()
    for arg_name in ('base_dir', 'models_dir',
                     'python_model_tests_dir', 'python_model_plots_dir'):
        project_paths[arg_name] = getattr(args, arg_prefix + arg_name)

    # Load existing project information, update it, store back to file
    projects = load_from_cfg_dir('projects')  # empty dict if file is missing
    projects[project_name] = project_paths

    write_to_cfg_dir('projects', projects)
    log.info("Updated information for Utopia project '%s'.", project_name)

    # TODO If python_model_plots_dir is given, update plot modules cfg file


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


def copy_model_files(*, model_name: str, new_name: str, models_dir: str,
                     prompt_for_confirmation: bool=False,
                     add_to_cmakelists: bool=True) -> None:
    """
    Args:
        model_name (str): Description
        new_name (str): Description
        models_dir (str): Description
        prompt_for_confirmation (bool, optional): Description
    
    Raises:
        ValueError: Description
    """
    def apply_replacements(s, *replacements: Sequence[Tuple[str, str]]) -> str:
        """Applies multiple replacements onto the given string"""
        for replacement in replacements:
            s = s.replace(*replacement)
        return s

    def create_file_map(*, files: Sequence[str],
                        source_dir: str, target_dir: str,
                        abs_file_map: dict,
                        replacements: Sequence[Tuple[str, str]]) -> dict:
        """Given a file list with absolute paths, aggregates the file path
        changes into ``abs_file_map`` and gathers the relative file path
        changes into the returned dict.
        The file name is changed according to the specified replacements.
        """
        rel_file_map = dict()

        for fpath in files:
            rel_fpath = os.path.relpath(fpath, start=source_dir)
            new_rel_fpath = apply_replacements(rel_fpath, *replacements)

            abs_file_map[fpath] = os.path.join(target_dir, new_rel_fpath)
            rel_file_map[rel_fpath] = new_rel_fpath

        return rel_file_map

    def prompt(question: str) -> bool:
        try:
            response = input("\n{} [y/N]  ".format(question))
        
        except KeyboardInterrupt:
            return False

        if response.lower() not in ['y', 'yes']:
            return False

        return True

    # Get the model information
    info_bundle = _get_info_bundle(model_name=model_name)

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

    # Define the replacements
    replacements = [
        (model_name, new_name),
        (model_name.lower(), new_name.lower()),
        (model_name.upper(), new_name.upper()),
    ]

    # Parse the target directory (making it absolute) and perform some
    # rudimentary checks that make sure it's a valid model directory
    models_dir = os.path.expanduser(models_dir)
    if not os.path.isabs(models_dir):
        models_dir = os.path.abspath(models_dir)

    if not os.path.isdir(models_dir):
        raise ValueError("The specified target model directory, {}, does not "
                         "exist!".format(models_dir))

    # TODO Check against Utopia config which should contain a list of all
    #      models source directories

    # The mapping of all files that are to be copied and in which the content
    # is to be replaced. It maps absolute source file paths to absolute target
    # file paths.
    file_map = dict()

    # Define the source and target directory paths of the implementation
    impl_source_dir = info_bundle.paths['source_dir']
    impl_target_dir = os.path.join(models_dir, new_name)
    impl_file_map = dict() # Contains relative paths

    impl_source_files = glob.glob(os.path.join(impl_source_dir, "*"),
                                 recursive=True)
    impl_file_map = create_file_map(files=impl_source_files,
                                    source_dir=impl_source_dir,
                                    target_dir=impl_target_dir,
                                    abs_file_map=file_map,
                                    replacements=replacements)

    # TODO Do the same for Python-related files


    # Gathered all information now. Inform about it.
    # Implementation-related files
    max_key_len = min(max([len(k) for k in impl_file_map]), 32)
    print("The following implementation files from\n\t{}\n\nwill be copied "
          "to\n\t{}\n\nusing the following new file names:"
          "".format(impl_source_dir, impl_target_dir))
    print("\n".join(["\t{:{l:d}s}  ->  {:s}".format(k, v, l=max_key_len)
                     for k, v in impl_file_map.items()]))

    # TODO Python-related files


    # Replacements
    max_repl_len = max([len(rs) for rs, _ in replacements])
    print("\nInside all of these {:d} files, the following string "
          "replacements will be made:\n{}\n"
          "".format(len(file_map),
                    "\n".join(["\t'{:{l:d}s}'  ->  '{:s}'"
                               "".format(*repl, l=max_repl_len)
                               for repl in replacements])))

    # Ask whether to proceed
    if prompt_for_confirmation and not prompt("Proceed with copying?"):
        print("Not proceeding ...")
        return
    print("Now copying ...")

    # Now, the actual copying
    for i, (src_fpath, target_fpath) in enumerate(file_map.items()):
        print("Copying file {:d}/{:d} ...".format(i+1, len(file_map)))
        print("\t   {:s}\n\t-> {:s}\n".format(src_fpath, target_fpath))

        with open(src_fpath, mode='r') as src_file:
            src_lines = src_file.read()

        # Apply the replacements
        target_lines = apply_replacements(src_lines, *replacements)

        # Write the file, failing if it already exists
        os.makedirs(os.path.dirname(target_fpath), exist_ok=True)
        with open(target_fpath, mode='x') as target_file:
            target_file.write(target_lines)

    print("Finished copying.")

    if not add_to_cmakelists:
        print("Remember to register the new model in the relevant "
              "CMakeLists.txt file and reconfigure using CMake.")
        return

    # Add subdirectory to CMakeLists.txt file
    print("Adding model directory to CMakeLists.txt ...")
    cmakelists_fpath = os.path.abspath(os.path.join(impl_target_dir,
                                                    "../CMakeLists.txt"))
    with open(cmakelists_fpath, 'r') as cmakelists_file:
        cmakelists_lines = cmakelists_file.readlines()

    # Find the relevant add_subdirectory command to add the line to.
    # Assuming an ascending alphabetical list, it need be added before the
    # first model name that compares larger to the new model name
    insert_idx = None
    for i, line in enumerate(cmakelists_lines):
        if line.startswith("add_subdirectory"):
            if line[len("add_subdirectory(")].lower() > new_name.lower():
                insert_idx = i
                break

    if insert_idx is None:
        raise ValueError("Found no add_subdirectory command to insert the "
                         "new model directory after; please do it manually!\n"
                         "File:  {}".format(cmakelists_fpath))

    cmakelists_lines.insert(insert_idx - 1,
                            "add_subdirectory({})\n".format(new_name))

    with open(cmakelists_fpath, 'w') as cmakelists_file:
        cmakelists_file.writelines(cmakelists_lines)

    print("Finished.")
