# Configure utopya with information on model targets
#
# .. cmake_function:: generate_model_info
#
#   This function calls 'configure_file' with the arguments given
#   after deleting a possibly already generated output file
#   to ensure that 'configure_file' is executed again.
#
function(generate_model_info file_in file_out)
    # remove existing file
    file(REMOVE ${file_out})

    # write new file
    string(TIMESTAMP CURRENT_TIME)
    configure_file(${file_in} ${file_out} @ONLY)
endfunction()