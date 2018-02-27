# install headers
install(FILES
            agent.hh
            cell.hh
            data.hh
            data_eps.hh
            data_vtk.hh
            entity.hh
            grid.hh
            neighborhoods.hh
            setup.hh
            simulation.hh
            test
            types.hh
            utopia.hh
            utopia_dune.hh
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/utopia)

add_subdirectory(test EXCLUDE_FROM_ALL)