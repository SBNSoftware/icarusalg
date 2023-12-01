/**
 * @file   TrackTimeInterval_test.cc
 * @brief  Unit test for `lar::util::TrackTimeInterval` with ICARUS geometry.
 * @date   May 19th, 2023
 * @author petrillo@slac.stanford.edu
 *
 * Usage: just run the executable.
 * Or plug a FHiCL file in the command line.
 * 
 * There is only one ICARUS-specific line in this code, that is the choice
 * of channel mapping. In fact, the test would probably work for ICARUS
 * even with the default channel mapping.
 */

// Boost test libraries; defining this symbol tells boost somehow to generate
// a main() function; Boost is pulled in by boost_unit_test_base.h
#define BOOST_TEST_MODULE TrackTimeIntervalTest

// ICARUS libraries
#include "icarusalg/Utilities/TrackTimeInterval.h"
#include "icarusalg/Geometry/ICARUSChannelMapAlg.h"

// LArSoft libraries
#include "lardataalg/DetectorInfo/DetectorPropertiesStandard.h"
#include "lardataalg/DetectorInfo/LArPropertiesStandard.h"
#include "lardataalg/DetectorInfo/DetectorClocksStandard.h"
#include "lardataalg/DetectorInfo/DetectorPropertiesStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/DetectorClocksStandardTestHelpers.h"
#include "lardataalg/DetectorInfo/LArPropertiesStandardTestHelpers.h"
#include "lardataalg/Utilities/quantities/spacetime.h" // microseconds
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/CoreUtils/counter.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcorealg/TestUtils/boost_unit_test_base.h"
#include "larcorealg/TestUtils/geometry_unit_test_base.h"
#include "lardataobj/RecoBase/Hit.h"

// standard C/C++ libraries
#include <stdexcept> // std::runtime_error
#include <iostream>
#include <random>
#include <vector>
#include <string>
#include <optional>
#include <cmath> // std::abs()
#include <cassert>


//------------------------------------------------------------------------------
//---  The test environment
//---

using TesterConfiguration =
  testing::BasicGeometryEnvironmentConfiguration<icarus::ICARUSChannelMapAlg>;
using TestEnvironment = testing::GeometryTesterEnvironment<TesterConfiguration>;


class TestFixture {

  // the initialization of the static object needs to be delayed
  // until the inizialization of an instance of the fixture happens.
  static std::optional<TestEnvironment> gEnv;
  
  static void initGlobalTestEnv()
    {
      int const argc = boost::unit_test::framework::master_test_suite().argc;
      auto const argv = boost::unit_test::framework::master_test_suite().argv;
      
      if (argc != 2)
        throw std::runtime_error("FHiCL configuration file path required as first argument!");
      
      TesterConfiguration config{ "TrackTimeIntervalTest" };
      int iParam = 0;
      config.SetConfigurationPath(argv[++iParam]);
      
      gEnv.emplace(config);
      TestEnvironment& testEnv = *gEnv;
      // DetectorPropertiesStandard and all its dependencies support the simple set
      // up (see testing::TesterEnvironment::SimpleProviderSetup()), except for
      // Geometry, that has been configured already in the geometry-aware
      // environment. So we invoke a simple set up for each of the dependencies:
      testEnv.SimpleProviderSetup<detinfo::LArPropertiesStandard>();
      testEnv.SimpleProviderSetup<detinfo::DetectorClocksStandard>();
      testEnv.SimpleProviderSetup<detinfo::DetectorPropertiesStandard>();
    }
  
    public:
  
  /// Setup: may initialize the global environment.
  TestFixture() { if (!gEnv) initGlobalTestEnv(); }

  /// Retrieves the global tester
  static TestEnvironment const& Env() { return gEnv.value(); }

}; // class TestFixture

std::optional<TestEnvironment> TestFixture::gEnv;


//------------------------------------------------------------------------------
namespace {
  
  recob::Hit makeHitAt(
    raw::ChannelID_t channel, float tick,
    geo::View_t view, geo::SigType_t signalType, geo::WireID const& wireID
  ) {
    return {
        channel            // channel
      , int(tick) - 20     // start_tick
      , int(tick) + 20     // end_tick
      , tick               // peak_time
      , 3.0                // sigma_peak_time
      , 4.0                // rms
      , 100.0              // peak_amplitude
      , 5.0                // sigma_peak_amplitude
      , 200                // summedADC
      , 200.0              // hit_integral
      , 10.0               // hit_sigma_integral
      , 1                  // multiplicity
      , 0                  // local_index
      , 1.0                // goodness_of_fit
      , 37                 // dof
      , view               // view
      , signalType         // signal_type
      , wireID             // wireID
      };
  } // makeHitAt()
  
} // local namespace

//------------------------------------------------------------------------------
//---  The tests
//---
//
// Note on Boost fixture options:
// - BOOST_FIXTURE_TEST_SUITE will provide the tester as a always-present data
//   member in the environment, as "Tester()"; but a new fixture, with a new
//   geometry and a new tester, is initialized on each test case
// - BOOST_GLOBAL_FIXTURE does not provide tester access, so one has to get it
//   as TestFixture::GlobalTester(); on the other hand, the
//   fixture is initialized only when a new global one is explicitly created.
//

// BOOST_FIXTURE_TEST_SUITE(TrackTimeIntervalTestEnv, TestFixture)
BOOST_GLOBAL_FIXTURE(TestFixture);

BOOST_AUTO_TEST_CASE(PrintHitsOnAllPlanes)
{
  auto const& testEnv = TestFixture::Env();
  auto const detClockData
    = testEnv.Provider<detinfo::DetectorClocks>()->DataForJob();
  
  geo::GeometryCore const& geom = *(testEnv.Provider<geo::GeometryCore>());
  detinfo::DetectorPropertiesData detProp
    = testEnv.Provider<detinfo::DetectorProperties>()->DataFor(detClockData);
  
  const int MaxTick = detProp.ReadOutWindowSize();
  
  lar::util::TrackTimeInterval const chargeTime
    { geom, detProp, detinfo::DetectorTimings{ detClockData } };
  
  for (geo::PlaneGeo const& plane: geom.Iterate<geo::PlaneGeo>()) {
    
    //
    // make some hits on this plane
    //
    std::vector<recob::Hit> hits;
    
    geo::WireID const wireID{ plane.ID(), 1 };
    raw::ChannelID_t const channel = geom.PlaneWireToChannel(wireID);
    geo::SigType_t const signalType = geom.SignalType(channel);
    geo::View_t const view = plane.View();
    
    int tick = 0;
    int tickStep = MaxTick / 16;
    while (tick <= MaxTick) {
      hits.push_back(makeHitAt(channel, tick, view, signalType, wireID));
      tick += tickStep;
    } // while
    
    //
    // print all the time ranges
    //
    std::cout << std::string("=", 80) << "\n" << plane.ID() << std::endl;
    for (recob::Hit const& hit: hits) {
      
      lar::util::TrackTimeInterval::TimeRange const& hitTimeRange
        = chargeTime.timeRange(hit);
      
      std::cout << hit.WireID() << " T=" << hit.PeakTime() << " => "
        << hitTimeRange << std::endl;
      
    } // for hits
    
  } // for planes
  
} // BOOST_AUTO_TEST_CASE(PrintHitsOnAllPlanes)


BOOST_AUTO_TEST_CASE(HitsOnPlanes)
{
  using electronics_time = detinfo::timescales::electronics_time;
  
  auto const& testEnv = TestFixture::Env();
  auto const detClockData
    = testEnv.Provider<detinfo::DetectorClocks>()->DataForJob();
  detinfo::DetectorTimings const detTiming{ detClockData };
  
  geo::GeometryCore const& geom = *(testEnv.Provider<geo::GeometryCore>());
  detinfo::DetectorPropertiesData detProp
    = testEnv.Provider<detinfo::DetectorProperties>()->DataFor(detClockData);
  
  lar::util::TrackTimeInterval const chargeTime{ geom, detProp, detTiming };
  
  electronics_time const triggerTime [[maybe_unused]] = detTiming.TriggerTime();
  
  double const driftVelocity = detProp.DriftVelocity();
  
  for (geo::TPCGeo const& TPC: geom.Iterate<geo::TPCGeo>()) {
    
    // refer to the first plane (ICARUS does that)
    geo::Point_t const firstPlaneCenter = TPC.FirstPlane().GetCenter();
    geo::Point_t const cathodeCenter = TPC.GetCathodeCenter();
    double const driftDistance
      = std::abs(cathodeCenter.X() - firstPlaneCenter.X());
    util::quantities::microseconds const driftTime
      { driftDistance / driftVelocity };
    
    for (geo::PlaneGeo const& plane: TPC.IteratePlanes()) {
      geo::WireID const wireID{ plane.ID(), 1 };
      raw::ChannelID_t const channel = geom.PlaneWireToChannel(wireID);
      geo::SigType_t const signalType = geom.SignalType(channel);
      geo::View_t const view = plane.View();
      
      // a hit at the anode at trigger time arrives immediately at trigger time;
      // that time can be also charge deposited at the cathode one drift time
      // earlier than the trigger time
      recob::Hit const anodeHit = makeHitAt(
        channel, detProp.ConvertXToTicks(firstPlaneCenter.X(), wireID),
        view, signalType, wireID
        );
      lar::util::TrackTimeInterval::TimeRange const anodeHitTimeRange
        = chargeTime.timeRange(anodeHit);

      BOOST_TEST(anodeHitTimeRange.start.value() == (triggerTime - driftTime).value(),
                 boost::test_tools::tolerance(0.001));
      BOOST_TEST(anodeHitTimeRange.stop.value() == triggerTime.value(),
                 boost::test_tools::tolerance(0.001));

      // a hit at the cathode at trigger time arrives one drift time later;
      // that time can be also charge deposited at the anode one drift time
      // after the trigger time
      recob::Hit const cathodeHit = makeHitAt(
        channel, detProp.ConvertXToTicks(cathodeCenter.X(), wireID),
        view, signalType, wireID
        );
      lar::util::TrackTimeInterval::TimeRange const cathodeHitTimeRange
        = chargeTime.timeRange(cathodeHit);

      BOOST_TEST(cathodeHitTimeRange.start.value() == triggerTime.value(),
                 boost::test_tools::tolerance(0.001));
      BOOST_TEST(cathodeHitTimeRange.stop.value() == (triggerTime + driftTime).value(),
                 boost::test_tools::tolerance(0.001));
      
    } // for planes
  } // for TPC
  
  
} // BOOST_AUTO_TEST_CASE(HitsOnPlanes)


BOOST_AUTO_TEST_CASE(PrintHitTimes)
{
  /*
   * Not much of a test: it mostly prints the allowed ranges for hits
   * reconstructed at different drift distances.
   * A single TPC is used as reference.
   * 
   */
  auto const& testEnv = TestFixture::Env();
  auto const detClockData
    = testEnv.Provider<detinfo::DetectorClocks>()->DataForJob();
  detinfo::DetectorTimings const detTiming{ detClockData };
  
  geo::GeometryCore const& geom = *(testEnv.Provider<geo::GeometryCore>());
  detinfo::DetectorPropertiesData detProp
    = testEnv.Provider<detinfo::DetectorProperties>()->DataFor(detClockData);
  
  lar::util::TrackTimeInterval const chargeTime{ geom, detProp, detTiming };
  
  double const driftVelocity = detProp.DriftVelocity(); // cm/us
  
  geo::TPCGeo const& TPC = geom.TPC(geo::TPCID{ 0, 0 });
    
  BOOST_TEST_MESSAGE("Hit times for " << TPC.ID());
  
  double const driftLength = std::abs(
    TPC.DriftDir().Dot(TPC.GetCathodeCenter() - TPC.FirstPlane().GetCenter())
    );
  
  geo::PlaneGeo const& plane = TPC.FirstPlane();
  geo::WireID::WireID_t const refWireNo = 100;
  
  double const xC = TPC.GetCathodeCenter().X();
  double const xA = plane.GetCenter().X();
  auto const xCoord = [x0=xA,L=xC-xA](double u){ return x0 + L * u; };
  
  raw::ChannelID_t const refChannel
    = geom.PlaneWireToChannel({ plane.ID(), 100 });
  geo::SigType_t const signalType = geom.SignalType(refChannel);
  geo::View_t const view = plane.View();
  
  std::default_random_engine engine; // default seed, not that random
  
  for (auto const i: util::counter(31)) {
    double const x = -1.0 + 0.1 * i;
    
    double const timeSpan = driftLength / driftVelocity; // us
    
    recob::Hit const hit = makeHitAt(
      refChannel + i, detProp.ConvertXToTicks(xCoord(x), plane.ID()),
        view, signalType, { plane.ID(), refWireNo + i }
        );
    
    lar::util::TrackTimeInterval::TimeRange const& timeRange
      = chargeTime.timeRange(hit);
    
    std::cout << "Hit at " << x << " of drift: time range: " << timeRange
      << std::endl;
    
    BOOST_TEST(timeRange.duration().value() == timeSpan,
               boost::test_tools::tolerance(0.001));
    
  } // for hit
  
} // BOOST_AUTO_TEST_CASE(PrintHitTimes)



BOOST_AUTO_TEST_CASE(timeRangeOfHits_singleTPCset)
{
  auto const& testEnv = TestFixture::Env();
  auto const detClockData
    = testEnv.Provider<detinfo::DetectorClocks>()->DataForJob();
  detinfo::DetectorTimings const detTiming{ detClockData };
  
  geo::GeometryCore const& geom = *(testEnv.Provider<geo::GeometryCore>());
  detinfo::DetectorPropertiesData detProp
    = testEnv.Provider<detinfo::DetectorProperties>()->DataFor(detClockData);
  
  // let's try the maker this time...
  lar::util::TrackTimeIntervalMaker const trackTimeIntervalMaker{ geom };
  lar::util::TrackTimeInterval const chargeTime
    = trackTimeIntervalMaker(detProp, detTiming);
  
  double const driftVelocity = detProp.DriftVelocity(); // cm/us
  
  for (geo::TPCGeo const& TPC: geom.Iterate<geo::TPCGeo>()) {
    
    BOOST_TEST_MESSAGE("Track test for " << TPC.ID());
    
    double const driftLength = std::abs(
      TPC.DriftDir().Dot(TPC.GetCathodeCenter() - TPC.FirstPlane().GetCenter())
      );
    
    geo::PlaneGeo const& plane = TPC.FirstPlane();
    geo::WireID::WireID_t const refWireNo = 100;
    
    double const xC = TPC.GetCathodeCenter().X();
    double const xA = plane.GetCenter().X();
    auto const xCoord = [x0=xA,L=xC-xA](double u){ return x0 + L * u; };
    
    raw::ChannelID_t const refChannel
      = geom.PlaneWireToChannel({ plane.ID(), 100 });
    geo::SigType_t const signalType = geom.SignalType(refChannel);
    geo::View_t const view = plane.View();
    
    std::default_random_engine engine; // default seed, not that random
    
    for (auto const [ Xa, Xb ]: {
        std::make_pair(0.2, 0.6)
      , std::make_pair(0.5, 0.8)
      , std::make_pair(-0.2, 0.1)
      , std::make_pair(0.0, 1.0)
      , std::make_pair(-0.2, 0.8)
      , std::make_pair(0.2, 1.2)
      , std::make_pair(-0.2, 1.2)
    }) {
      
      double const timeSpan
        = driftLength * (1.0 - std::abs(Xb - Xa)) / driftVelocity; // us
      
      constexpr std::size_t N = 5;
      std::array<recob::Hit, N> trackHits;
      double const step = (Xb - Xa)/(N-1);
      
      for (std::size_t i = 0; i < N; ++i) {
        double const x = Xa + i * step;
        trackHits[i] = makeHitAt(
          refChannel + i, detProp.ConvertXToTicks(xCoord(x), plane.ID()),
            view, signalType, { plane.ID(), refWireNo + i }
            );
      } // for hit positions
      
      for (int time = 0; time < 5; ++time) {
        std::shuffle(trackHits.begin(), trackHits.end(), engine);
        
        lar::util::TrackTimeInterval::TimeRange const& timeRange
          = chargeTime.timeRangeOfHits(trackHits);
        
        std::cout << "[" << time << " ] Track from " << Xa << " to " << Xb
          << " of drift: time range: " << timeRange << std::endl;
        
        BOOST_TEST(timeRange.duration().value() == timeSpan,
                   boost::test_tools::tolerance(0.001));
        
      } // for shuffles
      
    } // for track positions
    
  } // for all TPC
  
} // BOOST_AUTO_TEST_CASE(PrintHitsOnAllPlanes)



// BOOST_AUTO_TEST_SUITE_END()
