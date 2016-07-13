#install headers
install(FILES citcat.hh
			cell.hh
			data_vtk.hh
			setup.hh
			util.hh
			data.hh
			entity.hh
			simulation.hh
			types.hh
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/citcat)
