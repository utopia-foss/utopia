add_executable("toolbox" toolbox.cc)
target_link_dune_default_libraries("toolbox")

add_executable("cdm" cdm.cc)
target_link_dune_default_libraries("cdm")