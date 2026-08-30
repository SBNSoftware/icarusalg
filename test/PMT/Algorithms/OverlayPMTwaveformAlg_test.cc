/**
 * @file OverlayPMTwaveformAlg_test.cc
 * @brief Unit test for `sbn::opdet::OverlayPMTwaveformAlg`.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date March 27, 2026
 */

// Boost libraries
#define BOOST_TEST_MODULE OverlayPMTwaveformAlg
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp>  // BOOST_TEST(), BOOST_CHECK_EQUAL_COLLECTIONS()

// ICARUS libraries
#include "icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.h"

// ICARUS objects
#include "sbnobj/ICARUS/PMT/Data/WaveformBaseline.h"

// LArSoft libraries
#include "lardataobj/RawData/OpDetWaveform.h"

// C/C++ standard libraries
#include <cstdint>
#include <optional>
#include <vector>


//------------------------------------------------------------------------------
using namespace util::quantities::time_literals;


//------------------------------------------------------------------------------
using nanoseconds = util::quantities::intervals::nanoseconds;

using InputColl_t
  = std::vector<sbn::opdet::OverlayPMTwaveformAlg::InputWaveform_t>;

/*
 * The algorithm is designed to deal with pointers to source data (waveforms and
 * baselines). In this test, we need to provide storage for that data,
 * which would be normally provided by the event object.
 *  * `Input_t`/`makeInput()` pertain the storage
 *    (together waveform and its baseline for convenience)
 *  * `makeAlgInput()` creates references to the stored input suitable for the
 *    algorithm
 */
struct Input_t {
  raw::OpDetWaveform wf;
  icarus::WaveformBaseline baseline;
};


Input_t makeInput(
  double timeStamp_us, raw::Channel_t channel,
  std::vector<raw::ADC_Count_t> samples, raw::ADC_Count_t baseline
) {
  raw::OpDetWaveform waveform{ timeStamp_us, channel };
  waveform.Waveform() = std::move(samples);
  return Input_t{
    std::move(waveform),
    icarus::WaveformBaseline{ float(baseline) }
    };
} // makeInput()


sbn::opdet::OverlayPMTwaveformAlg::InputWaveform_t makeAlgInput
  (Input_t const& in)
{
  return { &in.wf, &in.baseline };
} // makeAlgInput()


void checkSamples(
  raw::OpDetWaveform const& out,
  std::vector<std::uint16_t> const& expected
  )
{
  auto const& got = out.Waveform();
  BOOST_CHECK_EQUAL_COLLECTIONS(
    got.begin(), got.end(),
    expected.begin(), expected.end()
    );
} // checkSamples()


sbn::opdet::OverlayPMTwaveformAlg makeAlg
  (nanoseconds tick, nanoseconds baselineCheck = 0_ns)
{
  sbn::opdet::OverlayPMTwaveformAlg::Config config;
  config.opticalTick = tick;
  config.baselineCheck = { baselineCheck, 0.0 };
  return sbn::opdet::OverlayPMTwaveformAlg{ config };
} // makeAlg()


//------------------------------------------------------------------------------
//--- Tests
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( SimContainsDataBothEnds_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  Input_t const data0 = makeInput(
    /*t*/ 0.0, /*ch*/ 0,
    /*samples*/ { 100, 100, 100, 100 },
    /*baseline*/ 100
    );

  Input_t const sim0 = makeInput(
    /*t*/ -2.0, /*ch*/ 0,
    /*samples*/ { 40, 40, 40, 40, 40, 40, 40, 40 }, // len=8 covers [-2,6)
    /*baseline*/ 50
    );

  InputColl_t const dataIn{ makeAlgInput(data0) };
  InputColl_t const simIn { makeAlgInput(sim0) };

  sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
    = alg.overlay(dataIn, simIn, std::nullopt);

  BOOST_TEST(res.waveforms.size() == 1U);

  BOOST_TEST_INFO_SCOPE("SimContainsDataBothEnds: waveform #0");
  checkSamples(res.waveforms[0], { 90, 90, 90, 90 });

} // SimContainsDataBothEnds_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( ManySimAroundOneData_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  Input_t const data0 = makeInput(
    /*t*/ 0.0, /*ch*/ 0,
    /*samples*/ { 100,100,100,100,100,100,100,100,100,100 }, // len=10: [0,10)
    /*baseline*/ 100
    );

  /*
   * Simulation waveforms are required to be non-overlapping in time.
   * baseline=50, samples=40 => contribution -10 wherever overlapping.
   *
   * Waveforms (all channel 0), in time order:
   *   [-6,-4)  no overlap
   *   [-4,-2)  no overlap (touches next at -2)
   *   [-2, 1)  overlaps only data sample 0
   *   [ 2, 4)  overlaps data samples 2,3
   *   [ 5, 7)  overlaps data samples 5,6
   *   [ 8,11)  overlaps data samples 8,9
   *   [11,12)  no overlap (touches previous at 11)
   *   [12,13)  no overlap
   */
  Input_t const s_before0 = makeInput(-6.0, 0, { 40, 40 }, 50);         // [-6,-4)
  Input_t const s_before1 = makeInput(-4.0, 0, { 40, 40 }, 50);         // [-4,-2)
  Input_t const s_ovStart = makeInput(-2.0, 0, { 40, 40, 40 }, 50);     // [-2, 1)
  Input_t const s_inside0 = makeInput( 2.0, 0, { 40, 40 }, 50);         // [ 2, 4)
  Input_t const s_inside1 = makeInput( 5.0, 0, { 40, 40 }, 50);         // [ 5, 7)
  Input_t const s_ovEnd   = makeInput( 8.0, 0, { 40, 40, 40 }, 50);     // [ 8,11)
  Input_t const s_after0  = makeInput(11.0, 0, { 40 }, 50);             // [11,12)
  Input_t const s_after1  = makeInput(12.0, 0, { 40 }, 50);             // [12,13)
  
  InputColl_t const dataIn{ makeAlgInput(data0) };
  InputColl_t const simIn{
    makeAlgInput(s_before0),
    makeAlgInput(s_before1),
    makeAlgInput(s_ovStart),
    makeAlgInput(s_inside0),
    makeAlgInput(s_inside1),
    makeAlgInput(s_ovEnd),
    makeAlgInput(s_after0),
    makeAlgInput(s_after1)
    };

  sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
    = alg.overlay(dataIn, simIn, std::nullopt);

  BOOST_TEST(res.waveforms.size() == 1U);

  BOOST_TEST_INFO_SCOPE("ManySimAroundOneData: waveform #0");
  checkSamples(res.waveforms[0], { 90, 100, 90, 90, 100, 90, 90, 100, 90, 90 });

} // ManySimAroundOneData_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( TouchingBoundariesVariants_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  Input_t const data0 = makeInput(
    /*t*/ 0.0, /*ch*/ 0,
    /*samples*/ { 100,100,100,100,100,100,100,100,100,100 }, // [0,10)
    /*baseline*/ 100
    );

  // A) adjacent before: [-2,0) -> no overlap
  Input_t const touchStartOutside = makeInput(-2.0, 0, { 40, 40 }, 50);

  // B) included touching start: [0,2) -> overlaps 0,1
  Input_t const touchStartInside = makeInput(0.0, 0, { 40, 40 }, 50);

  // C) adjacent after: [10,12) -> no overlap
  Input_t const touchEndOutside = makeInput(10.0, 0, { 40, 40 }, 50);

  // D) included touching end: [8,10) -> overlaps 8,9
  Input_t const touchEndInside = makeInput(8.0, 0, { 40, 40 }, 50);

  InputColl_t const dataIn{ makeAlgInput(data0) };

  { // A
    sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
      = alg.overlay(dataIn, { makeAlgInput(touchStartOutside) }, std::nullopt);
    BOOST_TEST_INFO_SCOPE("TouchingBoundariesVariants: touchStartOutside");
    checkSamples(res.waveforms[0], { 100,100,100,100,100,100,100,100,100,100 });
  }

  { // B
    sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
      = alg.overlay(dataIn, { makeAlgInput(touchStartInside) }, std::nullopt);
    BOOST_TEST_INFO_SCOPE("TouchingBoundariesVariants: touchStartInside");
    checkSamples(res.waveforms[0], { 90,90,100,100,100,100,100,100,100,100 });
  }

  { // C
    sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
      = alg.overlay(dataIn, { makeAlgInput(touchEndOutside) }, std::nullopt);
    BOOST_TEST_INFO_SCOPE("TouchingBoundariesVariants: touchEndOutside");
    checkSamples(res.waveforms[0], { 100,100,100,100,100,100,100,100,100,100 });
  }

  { // D
    sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
      = alg.overlay(dataIn, { makeAlgInput(touchEndInside) }, std::nullopt);
    BOOST_TEST_INFO_SCOPE("TouchingBoundariesVariants: touchEndInside");
    checkSamples(res.waveforms[0], { 100,100,100,100,100,100,100,100,90,90 });
  }

} // TouchingBoundariesVariants_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( TwoDataOneSimOverlapsBoth_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  Input_t const d0 = makeInput(0.0, 0, { 100,100,100,100 }, 100); // [0,4)
  Input_t const d1 = makeInput(3.0, 0, { 200,200,200,200 }, 200); // [3,7)

  Input_t const s0 = makeInput(2.0, 0, { 40,40,40,40 }, 50);      // [2,6)

  InputColl_t const dataIn{ makeAlgInput(d0), makeAlgInput(d1) };
  InputColl_t const simIn{ makeAlgInput(s0) };

  auto const res = alg.overlay(dataIn, simIn, std::nullopt);

  BOOST_TEST(res.waveforms.size() == 2U);

  BOOST_TEST_INFO_SCOPE("TwoDataOneSimOverlapsBoth: waveform #0");
  checkSamples(res.waveforms[0], { 100,100, 90, 90 });

  BOOST_TEST_INFO_SCOPE("TwoDataOneSimOverlapsBoth: waveform #1");
  checkSamples(res.waveforms[1], { 190,190,190,200 });

} // TwoDataOneSimOverlapsBoth_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( TwoDataTwoSimDisjointSameChannel_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  Input_t const d0 = makeInput(0.0,  0, { 100,100,100,100 }, 100); // [0,4)
  Input_t const d1 = makeInput(10.0, 0, { 200,200,200,200 }, 200); // [10,14)

  Input_t const s0 = makeInput(1.0,  0, { 40,40 }, 50);            // [1,3)
  Input_t const s1 = makeInput(11.0, 0, { 40,40 }, 50);            // [11,13)

  InputColl_t const dataIn{ makeAlgInput(d0), makeAlgInput(d1) };
  InputColl_t const simIn { makeAlgInput(s0), makeAlgInput(s1) };

  sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
    = alg.overlay(dataIn, simIn, std::nullopt);

  BOOST_TEST_INFO_SCOPE("TwoDataTwoSimDisjointSameChannel_testCase: waveform #0");
  checkSamples(res.waveforms[0], { 100, 90, 90,100 });

  BOOST_TEST_INFO_SCOPE("TwoDataTwoSimDisjointSameChannel_testCase: waveform #1");
  checkSamples(res.waveforms[1], { 200,190,190,200 });

} // TwoDataTwoSimDisjointSameChannel_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( NonOneMicrosecondTickOverlap_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(2_us);

  Input_t const d0 = makeInput(0.0, 0, { 100,100,100,100 }, 100); // [0,8) with tick=2us
  Input_t const s0 = makeInput(4.0, 0, { 40,40 }, 50);            // [4,8)

  sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
    = alg.overlay({ makeAlgInput(d0) }, { makeAlgInput(s0) }, std::nullopt);

  BOOST_TEST_INFO_SCOPE("NonOneMicrosecondTickOverlap: waveform #0");
  checkSamples(res.waveforms[0], { 100,100, 90, 90 });

} // NonOneMicrosecondTickOverlap_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOST_AUTO_TEST_CASE( TimestampSnappingRoundingArtifact_testCase ) {

  sbn::opdet::OverlayPMTwaveformAlg const alg = makeAlg(1_us);

  // Data starts at 1499.999 us but should snap to 1500 us.
  Input_t const d0 = makeInput(1499.999, 0, { 100,100 }, 100);

  // Sim starts exactly at 1500 us, overlaps first data sample after snapping.
  Input_t const s0 = makeInput(1500.0, 0, { 40 }, 50);

  sbn::opdet::OverlayPMTwaveformAlg::OverlaidWaveforms const res
    = alg.overlay({ makeAlgInput(d0) }, { makeAlgInput(s0) }, std::nullopt);

  BOOST_TEST_INFO_SCOPE("TimestampSnappingRoundingArtifact: waveform #0");
  checkSamples(res.waveforms[0], { 90, 100 });

} // TimestampSnappingRoundingArtifact_testCase


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
