/**
 * @file   icarusalg/Geometry/WireReadoutGeomICARUS.cxx
 * @brief  Channel mapping algorithms for ICARUS detector: implementation file.
 * @date   October 19, 2019
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see    icarusalg/Geometry/WireReadoutGeomICARUS.h
 */

// library header
#include "icarusalg/Geometry/WireReadoutGeomICARUS.h"

// ICARUS libraries
#include "icarusalg/Geometry/details/ROPandTPCsetBuildingAlg.h"

// LArSoft libraries
#include "larcorealg/Geometry/CryostatGeo.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/ReadoutDataContainers.h"
#include "larcorealg/Geometry/WireReadoutGeomBuilderStandard.h"
#include "larcorealg/Geometry/WireReadoutSorter.h"
#include "larcorealg/Geometry/details/extractMaxGeometryElements.h"
#include "larcorealg/CoreUtils/enumerate.h"
#include "larcorealg/CoreUtils/counter.h"
#include "larcorealg/CoreUtils/DebugUtils.h" // lar::debug::printBacktrace()
#include "larcorealg/CoreUtils/StdUtils.h" // util::size()
#include "larcoreobj/SimpleTypesAndConstants/geo_vectors.h"

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h" // cet::exception

// C/C++ libraries
#include <memory>
#include <vector>
#include <array>
#include <set>
#include <algorithm> // std::transform(), std::find()
#include <utility> // std::move()
#include <iterator> // std::back_inserter()
#include <functional> // std::mem_fn()
#include <tuple>


namespace {
  
  // ---------------------------------------------------------------------------
  
  // Creates a STL vector with the result of the transformation of `coll`.
  template <typename Coll, typename Op>
  auto transformCollection(Coll const& coll, Op op) {
    
    using Result_t
      = std::decay_t<decltype(op(std::declval<typename Coll::value_type>()))>;
    
    std::vector<Result_t> transformed;
    transformed.reserve(util::size(coll));
    std::transform
      (coll.begin(), coll.end(), std::back_inserter(transformed), op);
    return transformed;
  } // transformCollection()


  // ---------------------------------------------------------------------------
  
} // local namespace


// -----------------------------------------------------------------------------
icarus::WireReadoutGeomICARUS::WireReadoutGeomICARUS(Config const& config,
                                                     geo::GeometryCore const* geom,
                                                     std::unique_ptr<geo::WireReadoutSorter> sorter)
  : WireReadoutGeom{geom,
                    std::make_unique<geo::WireReadoutGeomBuilderStandard>(config.builder),
                    std::move(sorter)}
  , fWirelessChannelCounts
    (extractWirelessChannelParams(config.WirelessChannels()))
{
  // This is the only INFO level message we want this object to produce;
  // given the dynamic nature of the channel mapping choice,
  // it's better for the log to have some indication of chosen channel mapping.
  mf::LogInfo("WireReadoutGeomICARUS")
    << "Initializing WireReadoutGeomICARUS channel mapping algorithm.";
  
  buildReadoutPlanes(geom->Cryostats());
  
  fillChannelToWireMap(geom->Cryostats());
  
  MF_LOG_TRACE("WireReadoutGeomICARUS")
    << "WireReadoutGeomICARUS::Initialize() completed.";
  }


//------------------------------------------------------------------------------
std::vector<geo::WireID> icarus::WireReadoutGeomICARUS::ChannelToWire
  (raw::ChannelID_t channel) const
{
  //
  // input check
  //
  assert(!fPlaneInfo.empty());
  
  //
  // output
  //
  std::vector<geo::WireID> AllSegments;
  
  //
  // find the ROP with that channel
  //
  icarus::details::ChannelToWireMap::ChannelsInROPStruct const* channelInfo
    = fChannelToWireMap.find(channel);
  if (!channelInfo) {
    throw cet::exception("Geometry")
      << "icarus::WireReadoutGeomICARUS::ChannelToWire(" << channel
      << "): invalid channel requested (must be lower than "
      << Nchannels() << ")\n";
  }
  
  //
  // find the wire planes in that ROP
  //
  PlaneColl_t const& planes = ROPplanes(channelInfo->ropid);
  
  //
  // associate one wire for each of those wire planes to the channel
  //
  AllSegments.reserve(planes.size()); // this is sometimes (often?) too much
  for (geo::PlaneGeo const* plane: planes) {
    
    geo::PlaneID const& pid = plane->ID();
    ChannelRange_t const& channelRange = fPlaneInfo[pid].channelRange();
    
    if (!channelRange.contains(channel)) continue;
    AllSegments.emplace_back
      (pid, static_cast<geo::WireID::WireID_t>(channel - channelRange.begin()));
    
  } // for planes in ROP
  
  return AllSegments;
  
} // icarus::WireReadoutGeomICARUS::ChannelToWire()


//------------------------------------------------------------------------------
unsigned int icarus::WireReadoutGeomICARUS::Nchannels() const {
  
  return fChannelToWireMap.nChannels();
  
} // icarus::WireReadoutGeomICARUS::Nchannels()


//------------------------------------------------------------------------------
unsigned int icarus::WireReadoutGeomICARUS::Nchannels
  (readout::ROPID const& ropid) const 
{
  icarus::details::ChannelToWireMap::ChannelsInROPStruct const* ROPinfo
    = fChannelToWireMap.find(ropid);
  return ROPinfo? ROPinfo->nChannels: 0U;
} // icarus::WireReadoutGeomICARUS::Nchannels(ROPID)


//------------------------------------------------------------------------------
double icarus::WireReadoutGeomICARUS::WireCoordinate
  (double YPos, double ZPos, geo::PlaneID const& planeID) const
{
  /*
   * this should NOT be called... it shouldn't be here at all!
   */
  
  cet::exception e("WireReadoutGeomICARUS");
  e << "WireReadoutGeomICARUS does not support `WireCoordinate()` call."
    "\nPlease update calling software to use geo::PlaneGeo::WireCoordinate()`:"
    "\n";
  
  lar::debug::printBacktrace(e, 4U);
  
  throw e;
} // icarus::WireReadoutGeomICARUS::WireCoordinate()


//------------------------------------------------------------------------------
geo::WireID icarus::WireReadoutGeomICARUS::NearestWireID
  (const geo::Point_t& worldPos, geo::PlaneID const& planeID) const
{
  /*
   * this should NOT be called... it shouldn't be here at all!
   */
  
  cet::exception e("WireReadoutGeomICARUS");
  e << "WireReadoutGeomICARUS does not support `NearestWireID()` call."
    "\nPlease update calling software to use geo::PlaneGeo::NearestWireID()`:"
    "\n";
  
  lar::debug::printBacktrace(e, 3U);
  
  throw e;
} // icarus::WireReadoutGeomICARUS::NearestWireID()


//------------------------------------------------------------------------------
raw::ChannelID_t icarus::WireReadoutGeomICARUS::PlaneWireToChannel
  (geo::WireID const& wireID) const
{
  return fPlaneInfo[wireID].firstChannel() + wireID.Wire;
} // icarus::WireReadoutGeomICARUS::PlaneWireToChannel()


//------------------------------------------------------------------------------
std::set<geo::PlaneID> const& icarus::WireReadoutGeomICARUS::PlaneIDs() const {
  
  /*
   * this should NOT be called... it shouldn't be here at all!
   */
  
  cet::exception e("WireReadoutGeomICARUS");
  e << "WireReadoutGeomICARUS does not support `PlaneIDs()` call."
    "\nPlease update calling software to use geo::GeometryCore::IteratePlanes()`"
    "\n";
  
  lar::debug::printBacktrace(e, 3U);
  
  throw e;
  
} // icarus::WireReadoutGeomICARUS::PlaneIDs()


//------------------------------------------------------------------------------
unsigned int icarus::WireReadoutGeomICARUS::NTPCsets
  (readout::CryostatID const& cryoid) const
{
  return HasCryostat(cryoid)? TPCsetCount(cryoid): 0U;
} // icarus::WireReadoutGeomICARUS::NTPCsets()


//------------------------------------------------------------------------------
/// Returns the largest number of TPC sets any cryostat in the detector has
unsigned int icarus::WireReadoutGeomICARUS::MaxTPCsets() const {
  assert(fReadoutMapInfo);
  return fReadoutMapInfo.MaxTPCsets();
} // icarus::WireReadoutGeomICARUS::MaxTPCsets()


//------------------------------------------------------------------------------
/// Returns whether we have the specified TPC set
/// @return whether the TPC set is valid and exists
bool icarus::WireReadoutGeomICARUS::HasTPCset
  (readout::TPCsetID const& tpcsetid) const
{
  return
    HasCryostat(tpcsetid)? (tpcsetid.TPCset < TPCsetCount(tpcsetid)): false;
} // icarus::WireReadoutGeomICARUS::HasTPCset()


//------------------------------------------------------------------------------
readout::TPCsetID icarus::WireReadoutGeomICARUS::TPCtoTPCset
  (geo::TPCID const& tpcid) const
{
  return tpcid? TPCtoTPCset()[tpcid]: readout::TPCsetID{};
} // icarus::WireReadoutGeomICARUS::TPCtoTPCset()


//------------------------------------------------------------------------------
std::vector<geo::TPCID> icarus::WireReadoutGeomICARUS::TPCsetToTPCs
  (readout::TPCsetID const& tpcsetid) const
{
  std::vector<geo::TPCID> TPCs;
  if (!tpcsetid) return TPCs;
  
  auto const& TPClist = TPCsetTPCs(tpcsetid);
  TPCs.reserve(TPClist.size());
  std::transform(TPClist.begin(), TPClist.end(), std::back_inserter(TPCs),
    std::mem_fn(&geo::TPCGeo::ID)
    );
  return TPCs;
} // icarus::WireReadoutGeomICARUS::TPCsetToTPCs()


//------------------------------------------------------------------------------
geo::TPCID icarus::WireReadoutGeomICARUS::FirstTPCinTPCset
  (readout::TPCsetID const& tpcsetid) const
{
  if (!tpcsetid) return {};
  
  auto const& TPClist = TPCsetTPCs(tpcsetid);
  return TPClist.empty()? geo::TPCID{}: TPClist.front()->ID();
  
} // icarus::WireReadoutGeomICARUS::FirstTPCinTPCset()


//------------------------------------------------------------------------------
unsigned int icarus::WireReadoutGeomICARUS::NROPs
  (readout::TPCsetID const& tpcsetid) const
{
  return HasTPCset(tpcsetid)? ROPcount(tpcsetid): 0U;
} // icarus::WireReadoutGeomICARUS::NROPs()


//------------------------------------------------------------------------------
unsigned int icarus::WireReadoutGeomICARUS::MaxROPs() const {
  assert(fReadoutMapInfo);
  return fReadoutMapInfo.MaxROPs();
} // icarus::WireReadoutGeomICARUS::MaxROPs()

//------------------------------------------------------------------------------
bool icarus::WireReadoutGeomICARUS::HasROP(readout::ROPID const& ropid) const {
  return HasTPCset(ropid)? (ropid.ROP < ROPcount(ropid)): false;
} // icarus::WireReadoutGeomICARUS::HasROP()


//------------------------------------------------------------------------------
  /**
   * @brief Returns the ID of the ROP planeid belongs to, or invalid if none
   * @param planeid ID of the plane
   * @return the ID of the corresponding ROP, or invalid ID when planeid is
   *
   * In this mapping, readout planes and wire planes are mapped one-to-one.
   * The returned value mirrors the plane ID in the readout space.
   * If the plane ID is not valid, an invalid readout plane ID is returned.
   * Note that this check is performed on the validity of the plane ID, that
   * does not necessarily imply that the plane specified by the ID actually
   * exists.
   */
readout::ROPID icarus::WireReadoutGeomICARUS::WirePlaneToROP
  (geo::PlaneID const& planeid) const
{
  return planeid? PlaneToROP(planeid): readout::ROPID{};
} // icarus::WireReadoutGeomICARUS::WirePlaneToROP()


//------------------------------------------------------------------------------
std::vector<geo::PlaneID> icarus::WireReadoutGeomICARUS::ROPtoWirePlanes
  (readout::ROPID const& ropid) const
{
  std::vector<geo::PlaneID> Planes;
  if (!ropid) return Planes;
  
  auto const& PlaneList = ROPplanes(ropid);
  Planes.reserve(PlaneList.size());
  std::transform(PlaneList.begin(), PlaneList.end(), std::back_inserter(Planes),
    std::mem_fn(&geo::PlaneGeo::ID)
    );
  return Planes;
} // icarus::WireReadoutGeomICARUS::ROPtoWirePlanes()


//------------------------------------------------------------------------------
std::vector<geo::TPCID> icarus::WireReadoutGeomICARUS::ROPtoTPCs
  (readout::ROPID const& ropid) const
{
  std::vector<geo::TPCID> TPCs;
  if (!ropid) return TPCs;
  
  /*
   * We use the same algorithm as for extracting the plane IDs
   * (they implicitly convert to TPC ID... kind of them).
   * The algorithm does not test for duplication, i.e. in theory it could
   * produce lists with the same TPC ID being present multiple times
   * from different planes.
   * But this is not expected in this mapping, where each TPC holds at most
   * one wire plane for each view, and the planes in a ROP are all on the same
   * view. It might be matter of an assertion, but it's too complex to fit in an
   * `assert()` call.
   */
  auto const& PlaneList = ROPplanes(ropid);
  TPCs.reserve(PlaneList.size());
  std::transform(PlaneList.begin(), PlaneList.end(), std::back_inserter(TPCs),
    std::mem_fn(&geo::PlaneGeo::ID)
    );
  return TPCs;
} // icarus::WireReadoutGeomICARUS::ROPtoTPCs()


//------------------------------------------------------------------------------
readout::ROPID icarus::WireReadoutGeomICARUS::ChannelToROP
  (raw::ChannelID_t channel) const
{
  if (!raw::isValidChannelID(channel)) return {};
  icarus::details::ChannelToWireMap::ChannelsInROPStruct const* info
    = fChannelToWireMap.find(channel);
  return info? info->ropid: readout::ROPID{};
} // icarus::WireReadoutGeomICARUS::ChannelToROP()


//------------------------------------------------------------------------------
raw::ChannelID_t icarus::WireReadoutGeomICARUS::FirstChannelInROP
  (readout::ROPID const& ropid) const
{
  if (!ropid) return raw::InvalidChannelID;
  icarus::details::ChannelToWireMap::ChannelsInROPStruct const* info
    = fChannelToWireMap.find(ropid);
  return info? info->firstChannel: raw::InvalidChannelID;
} // icarus::WireReadoutGeomICARUS::FirstChannelInROP()


//------------------------------------------------------------------------------
geo::PlaneID icarus::WireReadoutGeomICARUS::FirstWirePlaneInROP
  (readout::ROPID const& ropid) const
{
  if (!ropid) return {};
  PlaneColl_t const& planes = ROPplanes(ropid);
  return planes.empty()? geo::PlaneID{}: planes.front()->ID();
} // icarus::WireReadoutGeomICARUS::FirstWirePlaneInROP()


//------------------------------------------------------------------------------
bool icarus::WireReadoutGeomICARUS::HasCryostat
  (readout::CryostatID const& cryoid) const
{
  assert(fReadoutMapInfo);
  return cryoid.Cryostat < fReadoutMapInfo.NCryostats();
} // icarus::WireReadoutGeomICARUS::HasCryostat()


//------------------------------------------------------------------------------
void icarus::WireReadoutGeomICARUS::fillChannelToWireMap
  (std::vector<geo::CryostatGeo> const& Cryostats)
{
  
  //
  // input check
  //
  assert(fReadoutMapInfo);
  assert(!Cryostats.empty());
  
  //
  // output setup
  //
  assert(fPlaneInfo.empty());
  std::array<unsigned int, 3U> maxSizes
    = geo::details::extractMaxGeometryElements<3U>(Cryostats, *this);

  fPlaneInfo.resize(maxSizes[0U], maxSizes[1U], maxSizes[2U]);
  
  
  raw::ChannelID_t nextChannel = 0; // next available channel
  
  // once again we do not have iteration facilities from `geo::GeometryCore`
  // available yet, so we go the nested loop way and bite it
  for (geo::CryostatGeo const& cryo: Cryostats) {
    
    readout::CryostatID const cid { cryo.ID() };
    
    auto const nTPCsets 
      = static_cast<readout::TPCsetID::TPCsetID_t>(TPCsetCount(cid));
    
    for (readout::TPCsetID::TPCsetID_t s: util::counter(nTPCsets)) {
      
      readout::TPCsetID const sid { cid, s };
      
      // select the channel count according to whether the TPC set is even or
      // odd; the selected structure is an array with one element for wire
      // plane signal type (first induction, second induction and collection):
      auto const& TPCsetChannelCounts
        = fWirelessChannelCounts.at(sid.TPCset & 1);
      
      auto const nROPs = static_cast<readout::ROPID::ROPID_t>(ROPcount(sid));
      
      for (readout::ROPID::ROPID_t r: util::counter(nROPs)) {
        
        mf::LogTrace log("WireReadoutGeomICARUS");
        
        readout::ROPID const rid { sid, r };
        auto const planeType = findPlaneType(rid);
        log << "ROP: " << rid
          << " (plane type: " << PlaneTypeName(planeType) << ")";
        
        auto const& WirelessChannelCounts
          = TPCsetChannelCounts.at(planeType);
        
        PlaneColl_t const& planes = ROPplanes(rid);
        log << " (" << planes.size() << " planes):";
        assert(!planes.empty());
        
        raw::ChannelID_t const firstROPchannel = nextChannel;
        
        auto iPlane = util::begin(planes);
        auto const pend = util::end(planes);
        
        // assign available channels to all wires of the first plane
        nextChannel += WirelessChannelCounts.first + (*iPlane)->Nwires();
        fPlaneInfo[(*iPlane)->ID()] = {
          ChannelRange_t
            { firstROPchannel + WirelessChannelCounts.first, nextChannel },
          rid
          };
        log << " [" << (*iPlane)->ID() << "] "
          << fPlaneInfo[(*iPlane)->ID()].firstChannel()
          << " -- " << fPlaneInfo[(*iPlane)->ID()].lastChannel() << ";";
        
        geo::Point_t lastWirePos = (*iPlane)->LastWire().GetCenter();
        
        while (++iPlane != pend) {
          
          geo::PlaneGeo const& plane = **iPlane;
          
          // find out which wire matches the last wire from the previous plane;
          // if there is no such wire, an exception will be thrown,
          // which is ok to us since we believe it should not happen.
          geo::WireID const lastMatchedWireID
            = plane.NearestWireID(lastWirePos);
          
          /*
          mf::LogTrace("WireReadoutGeomICARUS")
            << (*std::prev(iPlane))->ID() << " W:" << ((*std::prev(iPlane))->Nwires() - 1)
            << " ending at " << (*std::prev(iPlane))->LastWire().GetEnd()
            << " matched " << lastMatchedWireID
            << " which starts at " << plane.Wire(lastMatchedWireID).GetStart()
            ;
          */
          
          //
          // the first channel in this plane (`iPlane`) is the one associated
          // to the first wire in the plane, which has local wire number `0`;
          // the last channel from the previous plane (`nextChannel - 1`)
          // is associated to the matched wire (`lastMatchedWireID`),
          // which has some wire number (`lastMatchedWireID.Wire`).
          //
          auto const nWires = plane.Nwires();
          raw::ChannelID_t const firstChannel
            = (nextChannel - 1) - lastMatchedWireID.Wire;
          nextChannel = firstChannel + nWires;
          
          fPlaneInfo[plane.ID()] = { { firstChannel, nextChannel }, rid };
          log << " [" << plane.ID() << "] "
            << fPlaneInfo[plane.ID()].firstChannel() << " -- "
            << fPlaneInfo[plane.ID()].lastChannel() << ";";
          
          // update for the next iteration
          lastWirePos = plane.LastWire().GetCenter();
          
        } // while
        
        nextChannel += WirelessChannelCounts.second;
        unsigned int const nChannels = nextChannel - firstROPchannel;
        fChannelToWireMap.addROP(rid, firstROPchannel, nChannels);
        log
          << " => " << nChannels << " channels starting at " << firstROPchannel;
        
      } // for readout plane
      
    } // for TPC set
    
  } // for cryostat
  
  fChannelToWireMap.setEndChannel(nextChannel);
  mf::LogTrace("WireReadoutGeomICARUS")
    << "Counted " << fChannelToWireMap.nChannels() << " channels.";
  
} // icarus::WireReadoutGeomICARUS::fillChannelToWireMap()


// -----------------------------------------------------------------------------
void icarus::WireReadoutGeomICARUS::buildReadoutPlanes
  (std::vector<geo::CryostatGeo> const& Cryostats)
{
  // the algorithm is delegated:
  icarus::details::ROPandTPCsetBuildingAlg builder("WireReadoutGeomICARUS");
  
  auto results = builder.run(*this, Cryostats);
  
  fReadoutMapInfo.set(
    std::move(results).TPCsetCount(), std::move(results).TPCsetTPCs(),
    std::move(results).ROPcount(), std::move(results).ROPplanes(),
    std::move(results).TPCtoTPCset(), std::move(results).PlaneToROP()
    );
  
} // icarus::WireReadoutGeomICARUS::buildReadoutPlanes()


// -----------------------------------------------------------------------------
auto icarus::WireReadoutGeomICARUS::findPlaneType(readout::ROPID const& rid) const
  -> PlaneType_t
{
  /*
   * This implementation is very fragile, relying on the fact that the first
   * induction plane numbers are `kFirstInductionType`, the second induction
   * plane numbers are `kSecondInductionType` and the collection plane numbers
   * are `kCollectionType`. This assumption is not checked anywhere.
   * 
   */
  constexpr std::array PlaneTypes = { // let's C++ figure out type and size
    kFirstInductionType,  // P:0
    kSecondInductionType, // P:1
    kCollectionType       // P:2
  };
  
  PlaneColl_t const& planes = ROPplanes(rid);
  if (planes.empty()) return kUnknownType;
  if (auto const planeNo = planes.front()->ID().Plane; planeNo < PlaneTypes.size())
    return PlaneTypes[planeNo];
  else return kUnknownType;
  
} // icarus::WireReadoutGeomICARUS::findPlaneType()


// ----------------------------------------------------------------------------
geo::SigType_t icarus::WireReadoutGeomICARUS::SignalTypeForChannelImpl
  (raw::ChannelID_t const channel) const
{
  /*
   * We rely on the accuracy of `findPlaneType()` (which is admittedly less than
   * great) to assign signal type accordingly.
   */
  
  icarus::details::ChannelToWireMap::ChannelsInROPStruct const* channelInfo
    = fChannelToWireMap.find(channel);
  if (!channelInfo) return geo::kMysteryType;
  
  switch (findPlaneType(channelInfo->ropid)) {
    case kFirstInductionType:
    case kSecondInductionType:
      return geo::kInduction;
    case kCollectionType:
      return geo::kCollection;
    default:
      return geo::kMysteryType;
  } // switch
  
} // icarus::WireReadoutGeomICARUS::SignalTypeForChannelImpl()


// -----------------------------------------------------------------------------
auto icarus::WireReadoutGeomICARUS::extractWirelessChannelParams
  (Config::WirelessChannelStruct const& params) -> WirelessChannelCounts_t
{
  return {
    // even TPC sets (e.g. C:0 S:0)
    std::array{
      std::make_pair(
        params.FirstInductionPreChannels(),
        params.FirstInductionPostChannels()
        ),
      std::make_pair(
        params.SecondInductionEvenPreChannels(),
        params.SecondInductionEvenPostChannels()
        ),
      std::make_pair(
        params.CollectionEvenPreChannels(),
        params.CollectionEvenPostChannels()
        )
    },
    // odd TPC sets (e.g. C:0 S:1)
    std::array{
      std::make_pair(
        params.FirstInductionPreChannels(),
        params.FirstInductionPostChannels()
        ),
      std::make_pair(
        params.SecondInductionOddPreChannels(),
        params.SecondInductionOddPostChannels()
        ),
      std::make_pair(
        params.CollectionOddPreChannels(),
        params.CollectionOddPostChannels()
        )
    }
    };
  
} // icarus::WireReadoutGeomICARUS::extractWirelessChannelParams()


// ----------------------------------------------------------------------------
std::string icarus::WireReadoutGeomICARUS::PlaneTypeName(PlaneType_t planeType) {
  
  using namespace std::string_literals;
  switch (planeType) {
    case kFirstInductionType:  return "first induction"s;
    case kSecondInductionType: return "second induction"s;
    case kCollectionType:      return "collection induction"s;
    case kUnknownType:         return "unknown"s;
    default:
      return "unsupported ("s + std::to_string(planeType) + ")"s;
  } // switch
  
} // icarus::WireReadoutGeomICARUS::PlaneTypeName()


// ----------------------------------------------------------------------------
