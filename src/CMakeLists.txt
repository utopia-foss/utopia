add_executable("toolbox" toolbox.cc)
target_link_dune_default_libraries("toolbox")
# invoke debugging compile options on executable
target_compile_options("toolbox" PRIVATE -g -Wall)