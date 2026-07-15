include(CetTest)
# geometry test on ICARUS geometries
cet_transitive_paths(FHICL_DIR BINARY IN_TREE)
cet_test_env_prepend(FHICL_FILE_PATH "." ${TRANSITIVE_PATHS_WITH_FHICL_DIR} ${CMAKE_CURRENT_LIST_DIR} ${icarusalg_BINARY_DIR}/job ${icaruscode_BINARY_DIR}/job)
cet_transitive_paths(LIBRARY_DIR BINARY IN_TREE)
cet_test_env_prepend(CET_PLUGIN_PATH ${TRANSITIVE_PATHS_WITH_LIBRARY_DIR})
cet_transitive_paths(GDML_DIR BINARY IN_TREE)
cet_test_env_prepend(FW_SEARCH_PATH ${TRANSITIVE_PATHS_WITH_GDML_DIR})
#
# the following are tests equivalent to the module ones, but with no framework
#
# unit test (use the hard-coded channel mapping for ICARUS geometry)
cet_test(geometry_icarus_test
  SOURCE geometry_icarus_test.cxx
  TEST_ARGS test_geometry_icarus.fcl
  LIBRARIES icarusalg::Geometry
            larcorealg::Geometry
            larcorealg::GeometryTestLib
            messagefacility::MF_MessageLogger
            fhiclcpp::fhiclcpp
            cetlib_except::cetlib_except
	    ROOT::Core
#  OPTIONAL_GROUPS Broken
)


# unit test (use the hard-coded configuration for ICARUS geometry)
# this uses BOOST for the test
cet_test(geometry_iterator_icarus_test
  SOURCE geometry_iterator_icarus_test.cxx
  TEST_ARGS test_geometry_iterators_icarus.fcl
  LIBRARIES icarusalg::Geometry
            larcorealg::GeometryTestLib
            larcorealg::Geometry
            messagefacility::MF_MessageLogger
            fhiclcpp::fhiclcpp
            cetlib_except::cetlib_except
	    ROOT::Core
  USE_BOOST_UNIT
)


# unit test (use the hard-coded configuration for ICARUS geometry)
cet_test(geometry_iterator_loop_icarus_test
  SOURCE geometry_iterator_loop_icarus_test.cxx
  TEST_ARGS test_geometry_iterators_icarus.fcl
  LIBRARIES icarusalg::Geometry
            larcorealg::Geometry
            larcorealg::GeometryTestLib
            messagefacility::MF_MessageLogger
            fhiclcpp::fhiclcpp
            cetlib_except::cetlib_except
	    ROOT::Core
)



install_headers()
install_source()
