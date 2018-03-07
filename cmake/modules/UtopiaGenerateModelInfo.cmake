function(generate_model_info filename)
    # remove existing file
    file(REMOVE ${filename})

    # write new file
    string(TIMESTAMP CURRENT_TIME)
    configure_file(${filename}.in ${filename} @ONLY)
endfunction()