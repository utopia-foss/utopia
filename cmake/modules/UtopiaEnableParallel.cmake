#
# Add pre-processor definition that enables parallel features of Utopia to the
# targets listed as arguments.
#
# This setting is independent from the detection of dependencies required for
# parallel execution. If these dependencies are not found, using this function
# will have no effect on the parallel execution of the resulting program.
#
# However, parallel execution of the resulting program will only be enabled
# if this function is called on the target.
#
# This property is PRIVATE and will not propagate to linked targets!
#
function(enable_parallel)
    foreach(target ${ARGV})
        target_compile_definitions(${target} PRIVATE ENABLE_PARALLEL_STL)
    endforeach()
endfunction()
