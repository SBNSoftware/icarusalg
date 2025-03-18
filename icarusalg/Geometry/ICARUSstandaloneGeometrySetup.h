/**
 * @file   icarusalg/Geometry/ICARUSstandaloneGeometrySetup.h
 * @brief  Functions to facilitate ICARUS geometry initialization outside _art_.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   April 16, 2020
 * 
 * This is a header-only library.
 * It does include some static code (inlined).
 */

#ifndef ICARUSCODE_GEOMETRY_ICARUSSTANDALONEGEOMETRYSETUP_H
#define ICARUSCODE_GEOMETRY_ICARUSSTANDALONEGEOMETRYSETUP_H


// ICARUS libraries
#include "icarusalg/Geometry/ICARUSWireReadoutGeom.h"

// LArSoft libraries
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcorealg/Geometry/WireReadoutSorterStandard.h"
#include "larcorealg/Geometry/StandaloneGeometrySetup.h" // SetupReadout()

// framework libraries
#include "fhiclcpp/ParameterSet.h"

// C/C++ standard libraries
#include <memory> // std::make_unique()


namespace lar::standalone {
  
  // ---------------------------------------------------------------------------
  /// Specialization of `lar::standalone::SetupReadout()`
  /// for ICARUS wire readout (`icarus::ICARUSWireReadoutGeom`).
  template <>
  inline std::unique_ptr<geo::WireReadoutGeom>
  SetupReadout<geo::WireReadoutSorterStandard, icarus::ICARUSWireReadoutGeom>
    (fhicl::ParameterSet const& parameters, geo::GeometryCore const* geom)
  {
    using WireReadout = icarus::ICARUSWireReadoutGeom;
    using Sorter = geo::WireReadoutSorterStandard;
    
    std::unique_ptr<geo::WireReadoutSorter> wireReadoutSorter
      = std::make_unique<Sorter>
      (parameters.get<fhicl::ParameterSet>("SortingParameters"));
    
    auto const& mapperConfig = fhicl::Table<WireReadout::Config>{
      parameters.get<fhicl::ParameterSet>("Mapper", fhicl::ParameterSet{})
      }();
    
    return std::make_unique<WireReadout>
      (mapperConfig, geom, std::move(wireReadoutSorter));
    
  } // SetupReadout<WireReadoutSorterStandard,ICARUSWireReadoutGeom>()
  
  
  // ---------------------------------------------------------------------------
  
} // namespace lar::standalone


#endif // ICARUSCODE_GEOMETRY_ICARUSSTANDALONEGEOMETRYSETUP_H
