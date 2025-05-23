/**
 * @file   geometry_icarus_test.cxx
 * @brief  Unit test for geometry on ICARUS detector.
 * @date   October 10, 2017
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * 
 * Usage:
 *     
 *     geometry_icarus_test  [ConfigurationFile [GeometryTestParameterSet]]
 *     
 * By default, `GeometryTestParameterSet` is set to `"physics.analyzers.geotest"`.
 * 
 */

// ICARUS libraries
#include "icarusalg/Geometry/GeoObjectSorterPMTasTPC.h"
#include "icarusalg/Geometry/WireReadoutGeomICARUS.h"
#include "icarusalg/Geometry/WireReadoutSorterICARUS.h"
#include "test/Geometry/geometry_unit_test_icarus.h"
#include "test/Geometry/ChannelMapConfig.h"

// LArSoft libraries
#include "larcorealg/test/Geometry/GeometryTestAlg.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcorealg/Geometry/StandaloneGeometrySetup.h"

// utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"


//------------------------------------------------------------------------------
//---  The test environment
//---


// we define here all the configuration that is needed;
// we use an existing class provided for this purpose, since our test
// environment allows us to tailor it at run time.
using IcarusGeometryConfiguration
  = icarus::testing::IcarusGeometryEnvironmentConfiguration;

/*
 * GeometryTesterFixture, configured with the object above, is used in a
 * non-Boost-unit-test context.
 * It provides:
 * - `geo::GeometryCore const* Geometry()`
 * - `geo::GeometryCore const* GlobalGeometry()` (static member)
 */
using IcarusGeometryTestEnvironment
  = testing::GeometryTesterEnvironment<IcarusGeometryConfiguration, icarus::GeoObjectSorterPMTasTPC>;


//------------------------------------------------------------------------------
//---  The tests
//---

/** ****************************************************************************
 * @brief Runs the test
 * @param argc number of arguments in argv
 * @param argv arguments to the function
 * @return number of detected errors (0 on success)
 * @throw cet::exception most of error situations throw
 * 
 * The arguments in argv are:
 * 0. name of the executable ("Geometry_test")
 * 1. path to the FHiCL configuration file
 * 2. FHiCL path to the configuration of the geometry test
 *    (default: physics.analysers.geotest)
 * 3. FHiCL path to the configuration of the geometry
 *    (default: services.Geometry)
 * 
 */
//------------------------------------------------------------------------------
int main(int argc, char const** argv) {
  
  IcarusGeometryConfiguration config("geometry_test_ICARUS");
  config.SetMainTesterParameterSetName("geotest");
  
  //
  // parameter parsing
  //
  int iParam = 0;
  
  // first argument: configuration file (mandatory)
  if (++iParam < argc) config.SetConfigurationPath(argv[iParam]);
  
  // second argument: path of the parameter set for geometry test configuration
  // (optional; default: "physics.analysers.geotest");
  // if no path is provided, we have a empty default configuration;
  // if path is provided, we don't have any default configuration
  // and if the configuration is missing there will be an error
  if (++iParam < argc) config.SetMainTesterParameterSetPath(argv[iParam]);
  else                 config.AddDefaultTesterConfiguration("");
  
  // third argument: path of the parameter set for geometry configuration
  // (optional; default: "services.Geometry" from the inherited object)
  if (++iParam < argc) config.SetGeometryParameterSetPath(argv[iParam]);
  
  //
  // testing environment setup
  //
  IcarusGeometryTestEnvironment TestEnvironment(config);
  using namespace lar::standalone;
  auto const wireReadoutAlg =
    SetupReadout<geo::WireReadoutSorterICARUS, icarus::WireReadoutGeomICARUS>(wireReadoutConfig(TestEnvironment),
                                                                              TestEnvironment.Geometry());
  auto const auxDetGeom = SetupAuxDetGeometry(TestEnvironment.ServiceParameters("AuxDetGeometry"));
  
  //
  // run the test algorithm
  //
  
  // 1. we initialize it from the environment,
  geo::GeometryTestAlg Tester(TestEnvironment.Geometry(),
                              wireReadoutAlg.get(),
                              auxDetGeom.get(),
                              TestEnvironment.TesterParameters());
  
  // 2. then we run it!
  unsigned int nErrors = Tester.Run();
  
  // 3. And finally we cross fingers.
  if (nErrors > 0) {
    mf::LogError("geometry_test_ICARUS") << nErrors << " errors detected!";
  }
  
  return nErrors;
} // main()
