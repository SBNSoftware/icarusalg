/**
 * @file icarusalg/Geometry/LoadStandardICARUSgeometry.h
 * @brief Single-line utility to create `geo::GeometryCore` in non-art contexts.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * 
 * Provides `icarus::geo::LoadStandardICARUSgeometry()`.
 * 
 * This library is (intentionally and stubbornly) header-only.
 * It requires linking with:
 *  * `icarusalg_Geometry`
 *  * `larcorealg_Geometry`
 *  * `MF_MessageLogger`
 *  * `fhiclcpp`
 * 
 * (and, indirectly, more).
 * 
 */

#ifndef ICARUSALG_GEOMETRY_LOADSTANDARDICARUSGEOMETRY_H
#define ICARUSALG_GEOMETRY_LOADSTANDARDICARUSGEOMETRY_H


// ICARUS libraries
#include "icarusalg/Geometry/ICARUSstandaloneGeometrySetup.h"
#include "icarusalg/Geometry/ICARUSWireReadoutGeom.h"

// LArSoft and framework libraries
#include "larcorealg/Geometry/GeometryCore.h"
#include "messagefacility/MessageLogger/MessageLogger.h" // mf::StartMessageFacility()
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/ParameterSet.h"
#include "cetlib/filepath_maker.h"

// C/C++ libraries
#include <stdexcept> // std::runtime_error
#include <memory> // std::unique_ptr
#include <string>


// -----------------------------------------------------------------------------
namespace icarus::geo {
  std::unique_ptr<::geo::GeometryCore> LoadStandardICARUSgeometry [[nodiscard]]
    (std::string const& configPath);
} // namespace icarus::geo


// -----------------------------------------------------------------------------
// ---  inline implementation
// -----------------------------------------------------------------------------
/**
 * @brief Returns an instance of `geo::GeometryCore` with ICARUS geometry loaded
 * @param configPath path to a FHiCL configuration file including geometry
 * @return a unique pointer with `geo::GeometryCore` object
 * 
 * The geometry is initialized with the configuration found in the FHiCL file
 * pointed by `configPath`.
 * Within that file, the geometry service provider configuration table is
 * expected to be found as `services.Geometry` or, as fallback as `Geometry`.
 * If neither is present, the whole configuration will be used.
 * 
 * ICARUS geometry configuration has special conventions, which include:
 *  * a full `ChannelMapping` configuration in the `Geometry` configuration
 *    block, equivalent to the one passed to `WireReadout` service;
 *  * within it, a `tool_type` name.
 * 
 * The `ChannelMapping` table *must* be present in the configuration, and the
 * `tool_type` configuration atom must match
 * `ICARUSsplitInductionChannelMapSetupTool`. These parameters confirm that the
 * standard ICARUS geometry is intended.
 * 
 * If a configuration table `service.message` or `message` is found, message
 * facility is initialised with it, unless it is already running.
 * 
 * This utility is as simple to use as:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * std::unique_ptr<geo::GeometryCore> geom
 *   = icarus::geo::LoadStandardICARUSgeometry("standard_g4_icarus.fcl");
 * geom->Print(std::cout);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * It has been tested in Cling interpreter provided with ROOT 6.22/08.
 * 
 * Note that it is considered an error not to use the return value of this
 * function.
 * 
 */
inline std::unique_ptr<::geo::GeometryCore>
icarus::geo::LoadStandardICARUSgeometry [[nodiscard]]
  (std::string const& configPath)
{
  /*
   * 1. load the FHiCL configuration
   * 2. configuration check
   * 3. load the standard geometry
   * 4. return the geometry object
   */
  
  using namespace std::string_literals;
  
  // this is the name of the tool expected in the configuration
  static std::string const MagicToolName
    { "ICARUSsplitInductionChannelMapSetupTool"s };
  
  //
  // 1. load the FHiCL configuration
  //
  fhicl::ParameterSet config;
  {
    std::unique_ptr<cet::filepath_maker> const policy
      { cet::lookup_policy_selector{}.select("permissive", "FHICL_FILE_PATH") };
    fhicl::make_ParameterSet(configPath, *policy, config);
  }
  
  
  //
  // 2. configuration check
  //
  std::string mfConfigPath;
  if (!mf::isMessageProcessingSetUp()) {
    for (std::string const& path: { "services.message"s, "message"s }) {
      if (!config.is_key_to_table(path)) continue;
      mfConfigPath = path;
      break;
    }
  } // if no message facility yet
  
  std::string geomConfigPath;
  for (std::string const& path: { "services.Geometry"s, "Geometry"s }) {
    if (!config.is_key_to_table(path)) continue;
    geomConfigPath = path;
    break;
  }
  fhicl::ParameterSet const geomConfig = geomConfigPath.empty()
    ? config: config.get<fhicl::ParameterSet>(geomConfigPath);
  
  if (!geomConfig.is_key_to_table("ChannelMapping")) {
    throw std::runtime_error("icarus::geo::LoadStandardICARUSgeometry():"
      " FHiCL configuration does not have a `ChannelMapping` section"
      " (this is a ICARUS convention).\nConfiguration:\n"
      + std::string(80, '-') + '\n'
      + geomConfig.to_indented_string(1U)
      + std::string(80, '-') + '\n'
      );
  }
  
  std::string const wireReadoutpingToolType
    = geomConfig.get("ChannelMapping.tool_type", ""s);
  if (wireReadoutpingToolType != MagicToolName) {
    throw std::runtime_error("icarus::geo::LoadStandardICARUSgeometry() "
      ": unexpected value '" + wireReadoutpingToolType
      + "' for `ChannelMapping.tool_type` configuration parameter (expected: '"
      + MagicToolName + "').\nConfiguration:\n"
      + std::string(80, '-') + '\n'
      + geomConfig.to_indented_string(1U)
      + std::string(80, '-') + '\n'
      );
  }
  
  //
  // 3. load the standard geometry
  //
  
  // set up message facility (we can live without, output would go to std::cerr)
  if (!mfConfigPath.empty())
    mf::StartMessageFacility(config.get<fhicl::ParameterSet>(mfConfigPath));
  
  
  // 4. return the geometry object
  return SetupICARUSGeometry<icarus::ICARUSWireReadoutGeom>(geomConfig);
  
} // icarus::geo::LoadStandardICARUSgeometry()


#endif // ICARUSALG_GEOMETRY_LOADSTANDARDICARUSGEOMETRY_H
