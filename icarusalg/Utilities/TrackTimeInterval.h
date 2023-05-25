/**
 * @file icarusalg/Utilities/TrackTimeInterval.h
 * @brief Utilities to constrain the time of charge detected in the TPC.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date May 18, 2023
 * @see icarusalg/Utilities/TrackTimeInterval.cxx
 */

#ifndef ICARUSALG_UTILITIES_TRACKTIMEINTERVAL_H
#define ICARUSALG_UTILITIES_TRACKTIMEINTERVAL_H


// LArSoft libraries
#include "lardataalg/DetectorInfo/DetectorTimings.h"
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h"
#include "lardataalg/DetectorInfo/DetectorPropertiesData.h"
#include "lardataalg/Utilities/quantities/spacetime.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/readout_types.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"

// framework libraries
#include "canvas/Persistency/Common/Ptr.h"

// C/C++ standard libraries
#include <string>
#include <iterator> // std::cbegin(), std::cend()
#include <iosfwd>


// -----------------------------------------------------------------------------
namespace recob { class Hit; }

// -----------------------------------------------------------------------------
namespace lar::util {
  using namespace ::util::quantities::time_literals;
  class TrackTimeInterval;
  class TrackTimeIntervalMaker;
}
/**
 * @brief Returns the allowed time interval for TPC activity.
 * 
 * 
 * Example of usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * geo::GeometryCore const& geom = *(lar::providerFrom<geo::Geometry const>());
 * detinfo::DetectorTimings const detTimings
 *   { art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(event) };
 * detinfo::DetectorPropertiesData const& detProps{
 *   art::ServiceHandle<detinfo::DetectorPropertiesService>()->DataFor
 *     (event, detTimings.clockData())
 *   };
 * 
 * lar::util::TrackTimeInterval const chargeTime{ geom, detProps, detTimings };
 * 
 * auto const& trackToHits
 *   = event.getProduct<art::Assns<recob::Track, recob::Hit>>(trackTag);
 * 
 * for (auto const& [ trackPtr, hits ]
 *   : util::associated_groups_with_left(trackToHits))
 * {
 *   
 *   lar::util::TrackTimeInterval::TimeRange const hitTimeRange
 *     = chargeTime.timeRangeOfHits(hits);
 *   
 *   recob::Track const& track = *trackPtr;
 *   std::cout << "Track ID=" << track.ID() << " start=" << track.Start()
 *     << " cm, end=" << track.end() << " cm, is confined in time interval "
 *     << hitTimeRange << std::endl;
 *   
 * } // for
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * 
 */
class lar::util::TrackTimeInterval {
  
    public:
  
  using microseconds = ::util::quantities::intervals::microseconds;
  using electronics_time = detinfo::timescales::electronics_time;
  
  
  // ---  BEGIN  --- data structures -------------------------------------------
  /// Record to describe a time interval.
  struct TimeRange {
    
    /// Magic value for invalid times.
    static constexpr electronics_time UndefinedTime
      = std::numeric_limits<electronics_time>::lowest();
    
    /// Start of the interval (formally included).
    electronics_time start = UndefinedTime;
    
    /// End of the interval (formally excluded).
    electronics_time stop = UndefinedTime;
    
    /// Returns the extension of the range. It may be negative.
    constexpr electronics_time::interval_t duration() const
      { return stop - start; }
    
    /**
     * @brief Returns if `time` is contained in the range, with some `margin`.
     * @param time the time to be tested
     * @param margin (default: no margin) extend the interval by this much time
     * @return whether `time` is contained in the range within `margin`
     * 
     * If `margin` is negative, the interval is narrowed.
     * The `stop` time is not included in the range.
     * Inclusion in an invalid range is always `true`, because an invalid range
     * (also) represents an infinite range.
     */
    constexpr bool contains
      (electronics_time time, microseconds margin = 0_us) const
      { return contains(time, margin, margin); }
    
    /**
     * @brief Returns if `time` is contained in the range, with some margins.
     * @param time the time to be tested
     * @param startMargin the `start` is anticipated by this amount before check
     * @param stopMargin the `stop` is delayed by this amount before check
     * @return whether `time` is contained in the range within the margins
     * 
     * If a margin is negative, it narrows the range for the containment check.
     * The `stop` time is not included in the range.
     * Inclusion in an invalid range is always `true`, because an invalid range
     * (also) represents an infinite range.
     */
    constexpr bool contains
      (electronics_time time, microseconds startMargin, microseconds stopMargin)
      const;
    
    /// Returns whether the interval is valid
    /// (i.e. if at least one of the boundaries is defined).
    constexpr bool isValid() const
      { return (start != UndefinedTime) || (stop != UndefinedTime); }
    
    /// Returns whether the time range is empty.
    constexpr bool empty() const { return start >= stop; }
    
    /// Returns a string representing this time range: `[ start ; stop ]`
    /// or `<invalid>`.
    operator std::string() const;
    
    /// Contracts this range to its intersection with the `other` one,
    /// @return this object
    TimeRange& intersect(TimeRange const& other);
    
  }; // TimeRange
  
  // ---  END  ----- data structures -------------------------------------------
  
  
  /// Constructor: initializes with the specified detector properties.
  TrackTimeInterval(
    geo::GeometryCore const& geom,
    detinfo::DetectorPropertiesData detProp,
    detinfo::DetectorTimings detTimings
    );
  
  
  /**
   * @brief Returns the allowed time range for charge detected on `TPCtick`.
   * @param TPCtick tick at which the charge was detected in the TPC readout
   * @return an exact time range for the activity
   * 
   * 
   */
  TimeRange timeRange(double TPCtick, geo::PlaneID const& planeID) const;
  
  
  TimeRange timeRange(
    detinfo::timescales::TPCelectronics_tick_d tick,
    geo::PlaneID const& planeID
  ) const;
  
  /// Returns the time range for the hit
  /// (`recob::Hit::PeakTime()` is used as time).
  /// @see `timeRange(double) const`
  TimeRange timeRange(recob::Hit const& hit) const;
  
  
  /// Unfolds C pointers (returns an invalid range if `ptr` is null).
  template <typename T>
  TimeRange timeRange(T const* ptr) const
    { return ptr? timeRange(*ptr): TimeRange{}; }
  
  /// Unfolds _art_ pointers (returns an invalid range if `ptr` is null).
  template <typename T>
  TimeRange timeRange(art::Ptr<T> const& ptr) const
    { return timeRange(ptr.get()); }
  
  
  
  /**
   * @brief Returns the time range including all the times in a sequence.
   * @tparam BIter type of begin iterator
   * @tparam EIter type of end iterator
   * @param begin iterator to the sequence
   * @param end iterator past the end of the sequence
   * @return time range including all `recob::Hit` in the collection
   * 
   * All objects in the sequence must be acceptable arguments for a
   * `timeRange()` call.
   * 
   * For the hits in each drift volume (TPC set), the allowed time range is an
   * intersection of allowed time ranges from all the hits in that volume.
   * That range can be empty and also have "negative duration", `start`ing after
   * having `stop`ped: this means that the lowest stop is earlier than the
   * higher start.
   * 
   * If the hits span multiple drift volumes, they are combined using the
   * detector-dependent method `mergeTPCsetRanges_SBN()`.
   * 
   * If no element is present in the sequence, the time range is returned
   * invalid (`TimeRange::isValid()` returns `false`).
   */
  template <typename BIter, typename EIter>
  TimeRange timeRangeOfHits(BIter begin, EIter end) const;
  
  /**
   * @brief Returns the time range including all `recob::Hit` in the collection.
   * @tparam HitColl type of hit collection
   * @param hitColl the collection of hits
   * @return time range including all `recob::Hit` in the collection
   * @see `timeRange(BIter, EIter) const`
   * 
   * See `timeRange(BIter begin, EIter end) const` for details.
   * 
   * Type requirements
   * ------------------
   * 
   * `HitColl` must be a forward-iterable sequence whose elements are each one
   * compatible with a `timeRange()` call.
   * 
   */
  template <typename HitColl>
  TimeRange timeRangeOfHits(HitColl const& hits) const;
  
  
    private:
  friend class TrackTimeIntervalMaker;
  
  using centimeters = ::util::quantities::intervals::centimeters;
  struct TimeLimits_t {
    centimeters driftDistance;
  };
  
  struct GeometryCache_t {
    using LimitsCache_t = geo::PlaneDataContainer<TimeLimits_t>;
    using TPCtoSetMap_t = geo::TPCDataContainer<readout::TPCsetID>;
    
    /// Cached times for all the planes in the detector.
    LimitsCache_t limits;
    
    /// Cached dimensions for a TPC set data container.
    std::array<unsigned int, 2U> TPCsetDims;
    
    /// TPC to TPC set map.
    TPCtoSetMap_t TPCtoSet;
    
    /// Returns the cached number of cryostats in the detector.
    unsigned int Ncryostats() const { return TPCsetDims[0]; }
    
  }; // GeometryCache_t
  
  
  /// Constructor: uses a provided cache instead of creating it.
  TrackTimeInterval(
    GeometryCache_t geomCache,
    detinfo::DetectorPropertiesData detProp,
    detinfo::DetectorTimings detTimings
    );
  
  
  /// Local copy of detector properties.
  detinfo::DetectorPropertiesData const fDetProp;
  
  /// Local copy of the timing conversion utility (including `DetectorClocks`).
  detinfo::DetectorTimings const fDetTimings;
  
  double const fDriftVelocity; ///< Detector drift velocity [cm/us]
  
  GeometryCache_t const fGeomCache; ///< Cached geometry information.
  
  
  /// Creates a geometry cache.
  static GeometryCache_t buildGeomCache(geo::GeometryCore const& geom);
  
  /// Returns the time limits of all the planes in the specified TPC.
  static GeometryCache_t::LimitsCache_t extractTimeLimits
    (geo::GeometryCore const& geom);
  
  static GeometryCache_t::TPCtoSetMap_t extractTPCtoSetMap
    (geo::GeometryCore const& geom);
  
  
  /**
   * @brief Merges the ranges from all the TPC sets in the detector.
   * @param TPCsetRanges container of allowed range from each TPC set
   * @return a single allowed time range
   * 
   * As an input, each TPC set contributes its hypothesis of allowed time range;
   * that hypothesis is invalid (`!isValid()`) if there was no information in
   * the TPC set.
   * The combination assumes that all the hits belong to the same activity and
   * that the time of all that activity is only one.
   * Contributions from different cryostats are intersected, while within each
   * cryostat the combination is made in a detector-specific way (see
   * `mergeTPCsetRanges_SBN()`.
   */
  TimeRange mergeCathodeRanges
    (TimeRange const& range1, TimeRange const& range2) const;
  
  /**
   * @brief Merges two ranges from the opposite sides of a cathode.
   * @param range1 allowed time range from one side of the cathode
   * @param range2 allowed time range from the other side of the cathode
   * @return a combined range
   * 
   * If both ranges are valid, the combination assumes that the actual time is
   * locked and it is the time bringing both sides on the cathode (may result
   * in a range of times).
   * Otherwise, the range that is valid is returned as is.
   */
  TimeRange mergeTPCsetRanges_SBN
    (readout::TPCsetDataContainer<TimeRange> const& TPCsetRanges) const;
  
  
  /// Returns a copy of the wire ID of a `hit`.
  static geo::WireID hitWire(recob::Hit const& hit);
  
  /// Returns a copy of the wire ID of a `hit`.
  static geo::WireID hitWire(recob::Hit const* hit) { return hitWire(*hit); }
  
  /// Returns a copy of the wire ID of a hit.
  static geo::WireID hitWire(art::Ptr<recob::Hit> const& hitPtr)
     { return hitWire(*hitPtr); }
  
  /// Returns a `readout::TPCsetDataContainer` with the cached dimensions.
  template <typename T>
  readout::TPCsetDataContainer<T> makeTPCsetData() const;
  
  
}; // lar::util::TrackTimeInterval


namespace lar::util {
  /// Prints a time range to a text stream.
  std::ostream& operator<<
    (std::ostream& out, lar::util::TrackTimeInterval::TimeRange const& range);
}


// -----------------------------------------------------------------------------
/**
 * @brief Creates instances of `lar::util::TrackTimeInterval`.
 * 
 * `lar::util::TrackTimeInterval` classes require in principle per-event
 * information and they should be created on each new event.
 * They are designed to work with many hits at once, so they cache some
 * information to reduce the computing time, and part of that information is
 * not event-dependent.
 * 
 * To avoid the recalculation of that part of information, this class computes
 * it once at construction, and then it copies that to a new `TrackTimeInterval`
 * object any time one is requested.
 * This object should be declared as a data member of an analysis class
 * (an _art_ module, for example), and in the event loop it should be invoked
 * to get a usable `lar::util::TrackTimeInterval`.
 * 
 * Example of usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * class MyAnalysis: art::SharedAnalyzer {
 *   
 *   geo::GeometryCore const& fGeom;
 *   detinfo::DetectorClocksService const& fDetClocks;
 *   detinfo::DetectorPropertiesService const& fDetProp;
 *   
 *   lar::util::TrackTimeIntervalMaker const fTimeIntervalMaker;
 *   
 *   // ...
 *   
 *   MyAnalysis()
 *     : fGeom(*lar::providerFrom<geo::GeometryCore>())
 *     , fDetClocks(*art::ServiceHandle<detinfo::DetectorClocksService>())
 *     , fDetProp(*art::ServiceHandle<detinfo::DetectorPropertiesService>())
 *     , fTimeIntervalMaker{ fGeom }
 *     // ...
 *     {}
 *   
 *   void analyze(art::Event const& event) override;
 *   
 * };
 * 
 * 
 * void MyAnalysis::analyze(Event const& event) {
 *   
 *   detinfo::DetectorTimings const detTimings{ fDetClocks.DataFor(event) };
 *   detinfo::DetectorPropertiesData const& detProp
 *     { fDetProp.DataFor(event, detTimings.clockData()) };
 *   
 *   lar::util::TrackTimeInterval const timeIntervals
 *     = fTimeIntervalMaker(detProp, detTimings);
 *   
 *   // ... use timeIntervals
 *   
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * This is as opposed to _not_ using the maker:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * class MyAnalysis: art::SharedAnalyzer {
 *   
 *   geo::GeometryCore const& fGeom;
 *   detinfo::DetectorClocksService const& fDetClocks;
 *   detinfo::DetectorPropertiesService const& fDetProp;
 *   
 *   // ...
 *   
 *   MyAnalysis()
 *     : fGeom(*lar::providerFrom<geo::GeometryCore>())
 *     , fDetClocks(*art::ServiceHandle<detinfo::DetectorClocksService>())
 *     , fDetProp(*art::ServiceHandle<detinfo::DetectorPropertiesService>())
 *     // ...
 *     {}
 *   
 *   void analyze(art::Event const& event) override;
 *   
 * };
 * 
 * 
 * void MyAnalysis::analyze(Event const& event) {
 *   
 *   detinfo::DetectorTimings const detTimings{ fDetClocks.DataFor(event) };
 *   detinfo::DetectorPropertiesData const& detProp
 *     { fDetProp.DataFor(event, detTimings.clockData()) };
 *   
 *   lar::util::TrackTimeInterval const timeIntervals
 *     { geom, detProp, detTimings };
 *   
 *   // ... use timeIntervals
 *   
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * which does not benefit from the precomputed cache.
 */
class lar::util::TrackTimeIntervalMaker {
  
  /// Geometry cache to be copied to the `TrackTimeInterval` objects.
  TrackTimeInterval::GeometryCache_t const fGeomCache;
  
    public:
  
  /// Constructor: creates a cache from the geometry.
  TrackTimeIntervalMaker(geo::GeometryCore const& geom);
  
  //@{
  /// Returns a new `TrackTimeInterval` with the specified detector properties.
  TrackTimeInterval make(
    detinfo::DetectorPropertiesData detProp,
    detinfo::DetectorTimings detTimings
    ) const;
  
  TrackTimeInterval operator() (
    detinfo::DetectorPropertiesData detProp,
    detinfo::DetectorTimings detTimings
    ) const
    { return make(std::move(detProp), std::move(detTimings)); }
  //@}
  
}; // lar::util::TrackTimeIntervalMaker



// -----------------------------------------------------------------------------
// ---  Inline implementation
// -----------------------------------------------------------------------------
inline constexpr bool lar::util::TrackTimeInterval::TimeRange::contains
  (electronics_time time, microseconds startMargin, microseconds stopMargin)
  const
{
  return ((start == UndefinedTime) || (time >= start - startMargin))
    && ((stop == UndefinedTime) || (time < stop + stopMargin));
}


// -----------------------------------------------------------------------------
inline auto lar::util::TrackTimeInterval::timeRange
  (double TPCtick, geo::PlaneID const& planeID) const -> TimeRange
{
  return
    timeRange(detinfo::timescales::TPCelectronics_tick_d{ TPCtick }, planeID);
}


// -----------------------------------------------------------------------------
// ---  Template implementation
// -----------------------------------------------------------------------------
template <typename BIter, typename EIter>
auto lar::util::TrackTimeInterval::timeRangeOfHits(BIter begin, EIter end) const
  -> TimeRange
{
  auto TPCsetRanges = makeTPCsetData<TimeRange>();
  
  // per TPC set (i.e. drift volume)
  while (begin != end) {
    readout::TPCsetID const tpcsetID = fGeomCache.TPCtoSet.at(hitWire(*begin));
    TPCsetRanges[tpcsetID].intersect(timeRange(*begin++));
  }
  
  return mergeTPCsetRanges_SBN(TPCsetRanges);
} // lar::util::TrackTimeInterval::timeRangeOfHits(Iter)


// -----------------------------------------------------------------------------
template <typename HitColl>
auto lar::util::TrackTimeInterval::timeRangeOfHits(HitColl const& hits) const
  -> TimeRange
{
  using std::cbegin, std::cend;
  return timeRangeOfHits(cbegin(hits), cend(hits));
} // lar::util::TrackTimeInterval::timeRangeOfHits(HitColl)


// -----------------------------------------------------------------------------
template <typename T>
readout::TPCsetDataContainer<T> lar::util::TrackTimeInterval::makeTPCsetData
  () const
{
  return readout::TPCsetDataContainer<T>
    { fGeomCache.TPCsetDims[0], fGeomCache.TPCsetDims[1] };
} // lar::util::TrackTimeInterval::makeTPCsetData()

// -----------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_TRACKTIMEINTERVAL_H
