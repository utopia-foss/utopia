"""This CMake-configured module holds information about the non-python side of utopia.

Note that this is not a usual python module, but is merely used to hold this
information data and make it easily accessible to utopya.
"""

# Begin of CMake configured variables .........................................
_CMAKE_BINARY_DIR = "/Users/haraldmack/Development/Utopia/utopia/build-cmake"
_LAST_CONFIG_TIME = "2018-03-09T11:11:31"

_UTOPIA_MODEL_TARGETS = "dummy"
_UTOPIA_MODEL_BINPATHS = "/Users/haraldmack/Development/Utopia/utopia/build-cmake/dune/utopia/models/dummy"
# End of CMake configured variables ...........................................

# Need a function to parse the cmake variables
def parse_utopia_models(targets: str=_UTOPIA_MODEL_TARGETS, 
    binpaths: str=_UTOPIA_MODEL_BINPATHS) -> dict:
    """From the module constants, creates a dictionary with the model information in {model_name: model_binary_path} format.
    
    Args:
        targets (str, optional): The CMake-parsed, semi-colon separated list of
            utopia targets 
        binpaths (str, optional): The CMake-parsed, semi-colon separated list
            of utopia target binary paths
    
    Returns:
        dict: The model info dictionary
    """  

    if not targets or not binpaths:
        # No models present
        print(targets, binpaths)
        return {}

    # Populate model info with target names and model info dict containing binpath
    model_info = {t:dict(binpath=bp) for t, bp in zip(targets.split(";"),
                                                      binpaths.split(";"))}

    return model_info

# Now declare those variables that are 
MODELS = parse_utopia_models()
