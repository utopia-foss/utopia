
#
# Add compiler and linker flags for coverage reports to a list of targets
#
# Any target in the argument list will receive the required flags as private
# property.
#
function(add_coverage_flags)
    foreach(target ${ARGV})
        target_compile_options(${target} PRIVATE --coverage)
        target_link_libraries(${target} PRIVATE --coverage)
    endforeach()
endfunction()
