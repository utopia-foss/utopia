add_compile_options(-g -Wall)
add_executable("toolbox" toolbox.cc)
target_link_dune_default_libraries("toolbox")