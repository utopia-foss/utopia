#install headers
install(FILES citcat.hh
            cell.hh
            data_vtk.hh
            setup.hh
            data.hh
            entity.hh
            simulation.hh
            types.hh
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/citcat)

add_subdirectory(test EXCLUDE_FROM_ALL)