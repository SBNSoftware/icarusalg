/**
 * @file icarusalg/Geometry/GeoObjectSorterPMTasTPC.h
 * @brief  Geometry object sorter with PMT following TPC wire order.
 * @date   April 26, 2020
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see    icarusalg/Geometry/GeoObjectSorterPMTasTPC.cxx
 */

#ifndef ICARUSCODE_GEOMETRY_GEOOBJECTSORTERPMTASTPC_H
#define ICARUSCODE_GEOMETRY_GEOOBJECTSORTERPMTASTPC_H

// LArSoft libraries
#include "larcorealg/Geometry/GeoObjectSorterStandard.h"
#include "larcorealg/Geometry/fwd.h"
#include "larcorealg/CoreUtils/RealComparisons.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_vectors.h"

// framework libraries
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/ParameterSet.h"

// C++ standard library
#include <set>

// -----------------------------------------------------------------------------
namespace icarus { class GeoObjectSorterPMTasTPC; }

/**
 * @brief Geometry sorter having PMT channels follow the same order as TPC.
 * 
 * This class sorts the elements of the LArSoft detector description.
 * The TPC elements are sorted according to the "standard" algorithm
 * (`geo::GeoObjectSorterStandard`). The PMT are arranged so that their channels
 * mimic the order of the TPC channels (delegated to `icarus::PMTsorter`
 * algorithm).
 * 
 * The algorithm for assigning channels to the wires follows the criteria:
 * 
 * * TPC are ordered by increasing _x_ (related to drift direction);
 * * channels are assigned value ranges increasing with the TPC number,
 *   i.e. with increasing _x_ coordinate;
 * * within a wire plane, channel number increases with the _z_ (beam direction)
 *   coordinate of the wire(s) behind the channel;
 * * in case of same _z_ (as for ICARUS first induction plane), an increasing
 *   _y_ order (geographical vertical, toward the sky) is chosen.
 * 
 * PMT channels are assigned by a fixed LArSoft algorithm, cryostat by cryostat
 * with increasing cryostat number (first `C:0`, then `C:1`, ...).
 * Each cryostat has its set of optical detectors, sorted by a customizable
 * geometric sorting algorithm, and the channel number assignment follows the
 * sequence of optical detectors as sorted by that algorithm.
 * 
 * This class reimplements the geometric sorting algorithm following criteria
 * similar to the TPC wires:
 * 
 * * optical detectors are split by plane (_x_ direction);
 * * starting with the plane with lower _x_, optical detectors are sorted
 *   by _z_ coordinate, then by _y_ coordinate.
 * 
 * 
 * Configuration parameters
 * -------------------------
 * 
 * In addition to the parameters for the standard sorter
 * (`geo::GeoObjectSorterStandard`), this sorter supports the following
 * parameters:
 * 
 * * `OpDetSorter` (configuration table; default: empty): configures the
 *   PMT sorter object (see `icarus::PMTsorter` for details)
 * 
 */
class icarus::GeoObjectSorterPMTasTPC: public geo::GeoObjectSorterStandard {
  
    public:
  
  struct Config {
  
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;

    fhicl::Atom<double> ToleranceX {
      Name("ToleranceX"),
      Comment("tolerance when sorting optical detectors on x coordinate [cm]"),
      1.0 // default
      };

    fhicl::Atom<double> ToleranceY {
      Name("ToleranceY"),
      Comment("tolerance when sorting optical detectors on y coordinate [cm]"),
      1.0 // default
      };

  }; // Config

  struct KeysToIgnore {
    std::set<std::string> operator()() const { return {"tool_type"}; }
  };
  
  /// Constructor: passes the configuration to the base class.
  GeoObjectSorterPMTasTPC(fhicl::Table<Config, KeysToIgnore> const& config)
    : geo::GeoObjectSorterStandard(config.get_PSet())
    , fCmpX{ config().ToleranceX() }
    , fCmpY{ config().ToleranceY() }
    {}
  
  
  /**
   * @brief Sorts the specified optical detectors.
   * @param opDets collection of pointers to all optical detectors in a cryostat
   * 
   * The collection `opDets` of optical detectors is sorted in place.
   * Sorting criteria are documented in `icarus::GeoObjectSorterPMTasTPC` class
   * documentation.
   * 
   * This algorithm requires all optical detectors to have their center defined
   * (`geo::OpDetGeo::GetCenter()`). No other information is used.
   * 
   * @note The current implementation is very sensitive to rounding errors!
   * 
   */
  bool compareOpDets(geo::OpDetGeo const& od1, geo::OpDetGeo const& od2) const override;
  
    private:
  
  /// `geo::OpDetGeo` comparer according to one coordinate of their center.
  /// Accomodates for some tolerance.
  template <double (geo::Point_t::*Coord)() const>
  struct CoordComparer {
  
    /// Object used for comparison; includes a tolerance.
    lar::util::RealComparisons<double> const fCmp;
  
    /// Constructor: fixes the tolerance for the comparison.
    CoordComparer(double tol = 0.0): fCmp(tol) {}
  
    /// Returns whether `A` has a center coordinate `Coord` smaller than `B`.
    bool operator() (geo::Point_t const& A, geo::Point_t const& B) const
      {
        return fCmp.nonEqual((A.*Coord)(), (B.*Coord)());
      }

  }; // CoordComparer


  /// Sorting criterium according to _x_ coordinate of `geo::OpDetGeo` center.
  CoordComparer<&geo::Point_t::X> const fCmpX;

  /// Sorting criterium according to _y_ coordinate of `geo::OpDetGeo` center.
  CoordComparer<&geo::Point_t::Y> const fCmpY;

}; // icarus::GeoObjectSorterPMTasTPC


// -----------------------------------------------------------------------------

#endif // ICARUSCODE_GEOMETRY_GEOOBJECTSORTERPMTASTPC_H
