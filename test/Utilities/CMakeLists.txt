add_compile_options(-Wno-narrowing)
cet_test(rounding_test LIBRARIES cetlib::cetlib USE_BOOST_UNIT)
cet_test(ChangeMonitor_test LIBRARIES cetlib::cetlib USE_BOOST_UNIT)

cet_test(FastAndPoorGauss_test
  LIBRARIES
    larcorealg::CoreUtils
    ROOT::Core
    ROOT::Hist
    ROOT::RIO
    ROOT::MathCore
    cetlib::cetlib
  USE_BOOST_UNIT
  )

cet_test(SampledFunction_test LIBRARIES cetlib::cetlib larcorealg::CoreUtils icarusalg_Utilities  USE_BOOST_UNIT)

cet_test(FixedBins_test LIBRARIES cetlib::cetlib USE_BOOST_UNIT)
cet_test(IntegerRanges_test LIBRARIES cetlib::cetlib larcorealg::CoreUtils USE_BOOST_UNIT)

cet_test(BinningSpecs_test
  LIBRARIES
    icarusalg::Utilities
    cetlib::cetlib
  USE_BOOST_UNIT
  )
cet_test(NonRandomCounter_test
  LIBRARIES
    icarusalg::Utilities
    CLHEP::CLHEP
    canvas::canvas
    cetlib::cetlib
  USE_BOOST_UNIT
  )

macro(TrackTimeInterval_test_deactivated) # see SBNSoftware/icaruscode#666
cet_test(TrackTimeInterval_test USE_BOOST_UNIT
  TEST_ARGS -- standard_g4_icarus.fcl
  LIBRARIES PRIVATE
  icarusalg::Utilities
  icarusalg::Geometry
  lardataalg::DetectorInfo_TestHelpers
  lardataalg::DetectorInfo
  larcorealg::geometry_unit_test_base
  larcorealg::Geometry
  lardataobj::RecoBase
)
endmacro(TrackTimeInterval_test_deactivated)

cet_test(TimeIntervalConfig_test USE_BOOST_UNIT
  LIBRARIES PRIVATE
    icarusalg::Utilities
    lardataalg::DetectorInfo
    fhiclcpp::fhiclcpp
)

cet_test(sortLike_test
  LIBRARIES
    icarusalg::Utilities
  USE_BOOST_UNIT
  )
