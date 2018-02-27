# let's install deval into the venv using our own function!
python_install_package_remote(URL https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace.git@release/v0.9.9
TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)

# want to have packages installed in editable mode? call
# set(DUNE_PYTHON_INSTALL_EDITABLE True)

# if you have a python package in directory <package>, call
# dune_python_install_package(PATH <package>)