/**
 * @file icarusalg/Utilities/TrackTimeInterval.cxx
 * @brief Utilities to constrain the time of charge detected in the TPC.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date May 18, 2023
 * @see icarusalg/Utilities/TrackTimeInterval.h
 */


// library header
#include "icarusalg/Utilities/TrackTimeInterval.h"

// LArSoft libraries
#include "larcorealg/CoreUtils/counter.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireReadoutGeom.h"
#include "lardataobj/RecoBase/Hit.h"

// C/C++ standard libraries
#include <ostream>
#include <utility> // std::move()
#include <cmath> // std::abs()
#include <cassert>


// -----------------------------------------------------------------------------
lar::util::TrackTimeInterval::TrackTimeInterval(
  geo::GeometryCore const& geom,
  geo::WireReadoutGeom const& wireReadout,
  detinfo::DetectorPropertiesData detProp,
  detinfo::DetectorTimings detTimings
)
  : TrackTimeInterval
    { buildGeomCache(geom, wireReadout), std::move(detProp), std::move(detTimings) }
  {}


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::timeRange(recob::Hit const& hit) const
  -> TimeRange
{
  return timeRange(hit.PeakTime(), hit.WireID()); // that was easy...
}


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::timeRange(
  detinfo::timescales::TPCelectronics_tick_d TPCtick,
  geo::PlaneID const& planeID /* = ... */
) const -> TimeRange {
  /*
   * The time of arrival of some charge deposited at time t0 and at distance dX
   * from an anode plane is:
   *   t(P) = t0 + dt(D) = t0 + dX / v
   * where dt(D) is the time it takes the charge to drift to the plane and v is
   * the drift velocity, assumed uniform.
   * That time of arrival is directly converted into a tick R(P) by the TPC
   * readout electronics:
   *   R(P) = R(ref) + (t(P) - t(ref)) / T
   * where R(ref) is by construction the readout tick measured at the reference
   * time t(ref), t(ref) is on the same time scale as t0 and T is the tick
   * period (400 ns in ICARUS, 500 ns in MicroBooNE and others).
   * R(P) is also the tick reconstructed by the hit reconstruction,
   * `recob::Hit::PeakTime()`.
   * 
   * In this function we are interested in t0:
   *
   *     t(P) = t(ref) + T (R(P) - R(ref))
   *     t0 = t(P) - dX / v = t(ref) + T (R(P) - R(ref)) - dX / v
   *
   * We don't know what dX is and the point of this function is to return the
   * allowed interval of t0, which is covered by all the values of dX between
   * `0` (on the anode plane) and `D` (distance of the cathode from the anode
   * plane):
   * 
   *     t0(max) = t(ref) + T (R(P) - R(ref))
   *     t0 in [ t0(max) - D / v ; t0(max) ]
   *
   * t(ref) is the time of the trigger, so if we express t0 relative to the
   * trigger time we can omit it. The electronics time, on the other end,
   * does not refer to the trigger time.
   * `DetectorTimings` understands the time scale of R(P) (TPC electronics tick)
   * and can convert it to other scales, e.g. the "standard" electronics scale,
   * and give directly t0(max) in those other scales.
   */
  
  detinfo::timescales::electronics_time const time
    = fDetTimings.toElectronicsTime(TPCtick);
  
  TimeLimits_t const& planeTimeLimits = fGeomCache.limits[planeID];
  microseconds const driftTime{
    planeTimeLimits.driftDistance.convertInto<centimeters>().value()
    / fDriftVelocity
    };
  return { time - driftTime, time };
  
} // lar::util::TrackTimeInterval::timeRange(tick)


// -----------------------------------------------------------------------------
lar::util::TrackTimeInterval::TrackTimeInterval(
  GeometryCache_t geomCache,
  detinfo::DetectorPropertiesData detProp,
  detinfo::DetectorTimings detTimings
)
  : fDetProp{ std::move(detProp) }
  , fDetTimings{ std::move(detTimings) }
  , fDriftVelocity{ fDetProp.DriftVelocity() }
  , fGeomCache{ std::move(geomCache) }
  {}


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::buildGeomCache
  (geo::GeometryCore const& geom, geo::WireReadoutGeom const& wireReadout)
  -> GeometryCache_t
{
  return {
      extractTimeLimits(geom, wireReadout),             // limits
      { geom.Ncryostats(), wireReadout.MaxTPCsets() },  // TPCsetDims
      extractTPCtoSetMap(geom, wireReadout)             // TPCtoSet
    };
} // lar::util::TrackTimeInterval::buildGeomCache()


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::extractTimeLimits
  (geo::GeometryCore const& geom, geo::WireReadoutGeom const& wireReadout)
  -> GeometryCache_t::LimitsCache_t
{
  // this infrastructure and coding style is a remnant of a previous version
  // which needed more information per plane and that information was different
  // by plane; the following is still correct, has fewer assumptions and it's
  // easier to expand, but a bit more wordy.
  GeometryCache_t::LimitsCache_t limits{geom.Ncryostats(), geom.MaxTPCs(), wireReadout.MaxPlanes()};
  
  for (geo::TPCGeo const& TPC: geom.Iterate<geo::TPCGeo>()) {
    geo::PlaneGeo const& firstPlane = wireReadout.FirstPlane(TPC.ID());
    
    // in ICARUS the hit time is corrected as if it were on the first plane
    // (the most inner one);
    // TPC::DriftDistance() is based on the farthest plane,
    // so we compute our own instead
    geo::Point_t const& firstPlaneCenter = firstPlane.GetCenter();
    geo::Point_t const& cathodeCenter = TPC.GetCathodeCenter();
    
    centimeters const driftDistance
      { std::abs(TPC.DriftDir().Dot(cathodeCenter - firstPlaneCenter)) }; // cm
    
    for (geo::PlaneID const& planeID: wireReadout.Iterate<geo::PlaneID>(TPC.ID())) {
      
      limits[planeID] = { driftDistance };
      
    } // for planes
  } // for TPC
  
  return limits;
} // lar::util::TrackTimeInterval::extractTimeLimits()


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::extractTPCtoSetMap
  (geo::GeometryCore const& geom, geo::WireReadoutGeom const& wireReadout)
  -> GeometryCache_t::TPCtoSetMap_t
{
  GeometryCache_t::TPCtoSetMap_t map{geom.Ncryostats(), geom.MaxTPCs()};
  
  for (readout::TPCsetID const tpcsetID: wireReadout.Iterate<readout::TPCsetID>()) {
    assert(tpcsetID.isValid);
    for (geo::TPCID tpcID: wireReadout.TPCsetToTPCs(tpcsetID)) {
      assert(tpcID.isValid);
      map[tpcID] = tpcsetID;
    }
  } // for
  
  return map;
} // lar::util::TrackTimeInterval::extractTPCtoSetMap()


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::mergeTPCsetRanges_SBN
  (readout::TPCsetDataContainer<TimeRange> const& TPCsetRanges) const
  -> TimeRange
{
  // reduce to each cryostat
  std::vector<TimeRange> cryoRanges{ fGeomCache.Ncryostats() };
  for (readout::CryostatID::CryostatID_t const cryoNo
    : ::util::counter(cryoRanges.size())
  ) {
    readout::CryostatID const cryoID{ cryoNo };
    
    unsigned int const NTPCsets = fGeomCache.TPCsetDims[1];
    
    TimeRange& cryoRange = cryoRanges[cryoNo];
    
    // how do we combine time ranges from different TPCs?
    // it depends on the layout; I am far from willing to write a generic
    // algorithm for it, so here it goes the: SBN way!!
    // This assumes that TPC sets 0 and 1 are around a cathode;
    // and we don't support more than that.
    
    unsigned int const nCathodes = (NTPCsets + 1) / 2;
    // the code below would work with more, but the choice is not well-motivated
    assert(nCathodes == 1);
    
    for (unsigned int const TPCsetNo: ::util::counter(nCathodes)) {
      
      readout::TPCsetID const tpcsetID1(cryoID, TPCsetNo);
      readout::TPCsetID const tpcsetID2(cryoID, TPCsetNo + 1);
      cryoRange.intersect(mergeCathodeRanges(
        TPCsetRanges.hasTPCset(tpcsetID1)? TPCsetRanges[tpcsetID1]: TimeRange{},
        TPCsetRanges.hasTPCset(tpcsetID2)? TPCsetRanges[tpcsetID2]: TimeRange{}
        ));
      
    } // for cathodes
    
  } // for cryostats
  
  // reduce to a single value
  // if the time of the particle is one, then it must be the same in the
  // different cryostats; so... yep, we once more intersect
  
  TimeRange mergedRange;
  for (TimeRange const& range: cryoRanges) mergedRange.intersect(range);
  
  return mergedRange;
} // lar::util::TrackTimeInterval::mergeTPCsetRanges_SBN()


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::mergeCathodeRanges
  (TimeRange const& range1, TimeRange const& range2) const -> TimeRange
{

  if (!range2.isValid()) return range1; // even if range1 is itself invalid
  if (!range1.isValid()) return range2;
  
  // there are two ranges at the two sides of the cathode;
  // that pretty much means that the time for which the track hits touch the
  // cathode is the right one (that is, the lowest value, "start");
  // of course that cathode time may be not equal in the two drift volumes,
  // because nothing ever goes as it should;
  // so we give the range between the two times.
  
  electronics_time const time1 = range1.start;
  electronics_time const time2 = range2.start;
  return { std::min(time1, time2), std::max(time1, time2) };
} // lar::util::TrackTimeInterval::mergeCathodeRanges()


// -----------------------------------------------------------------------------
geo::WireID lar::util::TrackTimeInterval::hitWire(recob::Hit const& hit)
  { return hit.WireID(); }


// -----------------------------------------------------------------------------
// --- lar::util::TrackTimeInterval::TimeRange implementation
// -----------------------------------------------------------------------------
auto lar::util::TrackTimeInterval::TimeRange::intersect(TimeRange const& other)
  -> TimeRange&
{
  if ((start == UndefinedTime)
    || ((other.start != UndefinedTime) && (other.start > start))
  ) {
    start = other.start;
  }
  
  if ((stop == UndefinedTime)
    || ((other.stop != UndefinedTime) && (other.stop < stop))
  ) {
    stop = other.stop;
  }
  
  return *this;
} // lar::util::TrackTimeInterval::TimeRange::intersect(TimeRange)


// -----------------------------------------------------------------------------
lar::util::TrackTimeInterval::TimeRange::operator std::string() const {
  using namespace std::string_literals;
  return isValid()
    ? "[ "s
      + ((start == UndefinedTime)? "..."s: to_string(start))
      + " ; "
      + ((stop == UndefinedTime)? "..."s: to_string(stop))
      + " ]"
    : "<invalid>"s
    ;
} // lar::util::operator<<(TrackTimeInterval::TimeRange)


// -----------------------------------------------------------------------------
std::ostream& lar::util::operator<<
  (std::ostream& out, TrackTimeInterval::TimeRange const& range)
{
  // if this ever becomes a performance issue, the function may be reimplemented
  // to directly pour into the stream
  return out << std::string(range);
} // lar::util::operator<<(TrackTimeInterval::TimeRange)


// -----------------------------------------------------------------------------
// ---  lar::util::TrackTimeIntervalMaker
// -----------------------------------------------------------------------------
lar::util::TrackTimeIntervalMaker::TrackTimeIntervalMaker
  (geo::GeometryCore const& geom, geo::WireReadoutGeom const& wireReadout)
  : fGeomCache{ TrackTimeInterval::buildGeomCache(geom, wireReadout) }
  {}


// -----------------------------------------------------------------------------
auto lar::util::TrackTimeIntervalMaker::make(
  detinfo::DetectorPropertiesData detProp,
  detinfo::DetectorTimings detTimings
) const -> TrackTimeInterval
{
  return
    TrackTimeInterval{ fGeomCache, std::move(detProp), std::move(detTimings) };
}


// -----------------------------------------------------------------------------
