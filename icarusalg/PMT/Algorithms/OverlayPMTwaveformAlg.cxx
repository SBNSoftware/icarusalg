/**
 * @file   icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.cxx
 * @brief  Algorithm overlaying simulated PMT waveforms on data PMT waveforms.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   March 23, 2026
 * @see    icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.h
 */

#include "icarusalg/PMT/Algorithms/OverlayPMTwaveformAlg.h"

// ICARUS libraries
#include "icarusalg/Utilities/OpDetWaveformUtils.h" // raw::OrderOpDetWaveformByChannelAndTime

// C/C++ standard libraries
#include <algorithm> // std::lower_bound(), std::sort(), std::clamp()
#include <cassert>
#include <cmath> // std::llround()
#include <limits>
#include <string>
#include <type_traits> // std::is_signed_v
#include <vector>


//------------------------------------------------------------------------------
namespace {
  
  /// Returns a copy of `coll` sorted with the specified predicate `pred`.
  template <typename T, typename Pred>
  auto sorted(std::vector<T> coll, Pred&& pred) {
    using std::begin, std::end;
    std::sort(begin(coll), end(coll), pred);
    return coll;
  } // sorted()
  
} // local namespace


//------------------------------------------------------------------------------
//---  sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t
//------------------------------------------------------------------------------
// helper for quick time lookup of time-sorted waveforms
class sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t {
  
    public:
  
  static constexpr raw::Channel_t InvalidChannel
    = std::numeric_limits<raw::Channel_t>::max();
  
  struct SimEntry_t {
    InputWaveform_t wf;
    Interval_t interval;
  };
  
  /// Constant iterator to a `SimEntry_t` in our record.
  using SimEntryIter_t = std::vector<SimEntry_t>::const_iterator;
  
  struct SimEntryCRange_t {
    SimEntryIter_t b, e;
    SimEntryIter_t begin() const noexcept { return b; }
    SimEntryIter_t end() const noexcept { return e; }
  };
  
  /// Comparer for waveforms, times and `SimEntry_t`.
  struct EntryCmp {
    static electronics_time timeOf(electronics_time t) noexcept { return t; }
    static electronics_time timeOf(SimEntry_t const& s) noexcept
      { return s.interval.stop; }
    template <typename T, typename U>
    bool operator() (T&& a, U&& b) const noexcept { return timeOf(a) < timeOf(b); }
  };
  
  
  /// Returns the channel in this set of entries, or `InvalidChannel` if none.
  raw::Channel_t channel() const noexcept
    { return fEntries.empty()? InvalidChannel: fEntries.front().wf.waveform->ChannelNumber(); }
  
  SimEntryIter_t end() const { return fEntries.cend(); }
  
  /// Returns the first waveform from `start` that is not completely earlier than `t`.
  SimEntryIter_t nextAtTime(electronics_time t, SimEntryIter_t start) const;
  
  /// Returns the first waveform that is not completely earlier than `t`.
  SimEntryIter_t nextAtTime(electronics_time t) const
    { return nextAtTime(t, fEntries.cbegin()); }
  
  /// Returns a range of waveforms overlapping the specified interval.
  SimEntryCRange_t overlappingWaveforms(Interval_t interval) const;
  
  /// Appends a copy of the entry to the end.
  /// @throw cet::exception if not sorted in time
  void append(SimEntry_t entry);
  
    private:
  
  std::vector<SimEntry_t> fEntries; // keep it sorted!
  
}; // sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t::nextAtTime
  (electronics_time t, SimEntryIter_t start) const -> SimEntryIter_t
{
  // lookup is based on stop time:
  // it = std::upper_bound(t) finds the first element that is strictly after t,
  // that means it->stop > t
  //  1. if t is earlier than all waveforms, it will return `start`
  //  2. if t is inside a waveform, it will return it
  //  2.1 if inside many waveforms, it will return the one with the earliest stop
  //  3. if t matches the end (stop) of a waveform, it will return the next one
  //  4. if t is in between waveforms, it will return the one after t
  //  5. if t is after everything, it will return end()
  return std::upper_bound(start, fEntries.cend(), t, EntryCmp{});
}


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t::overlappingWaveforms
  (Interval_t interval) const -> SimEntryCRange_t
{
  SimEntryIter_t const eend = end();
  
  if (interval.empty()) return { eend, eend };
  
  SimEntryIter_t const start = nextAtTime(interval.start, fEntries.begin());
  SimEntryIter_t stop = nextAtTime(interval.stop, start);
  if ((stop != eend) && !stop->interval.after(interval)) {
    ++stop; // include this one in an open range
  }
  
  return { start, stop };
  
} // sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t::overlappingWaveforms()


//------------------------------------------------------------------------------
void sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t::append
  (SimEntry_t entry)
{
  assert(entry.wf.waveform);
  
  if (entry.interval.empty()) {
    throw cet::exception{ "OverlayPMTwaveformAlg" }
      << "Empty waveform added in cache for channel: CH=" << channel()
      << " TS=" << entry.wf.waveform->TimeStamp() << "\n";
  }
  
  if (!fEntries.empty()) { // check compatibility of new entry with previous one
    
    SimEntry_t const& previous = fEntries.back();
    
    if (entry.wf.waveform->ChannelNumber() != channel()) {
      throw cet::exception{ "OverlayPMTwaveformAlg" }
        << "Waveform added in cache for wrong channel: CH="
        << entry.wf.waveform->ChannelNumber() << " in cache for CH="
        << channel() << "\n";
    }
    
    // we do not support overlapping waveforms (it makes lookup more complicate)
    if (!entry.interval.after(previous.interval)) {

      if (entry.interval.overlaps(previous.interval)) {
        throw cet::exception{ "OverlayPMTwaveformAlg" }
          << "Overlapping waveform added to cache for CH=" << channel() << ": "
          << entry.interval << " on top of " << previous.interval
          << "\n";
      }
      else {
        throw cet::exception{ "OverlayPMTwaveformAlg" }
          << "Waveform added in cache out of time for CH=" << channel() << ": "
          << entry.interval << " after " << previous.interval
          << "\n";
      }
    }
    
  } // if not empty
  
  fEntries.push_back(std::move(entry));
} // sbn::opdet::OverlayPMTwaveformAlg::ChannelSimEntries_t::append()


//------------------------------------------------------------------------------
//---  sbn::opdet::OverlayPMTwaveformAlg
//------------------------------------------------------------------------------
sbn::opdet::OverlayPMTwaveformAlg::OverlayPMTwaveformAlg(Config const& config)
  : fOpticalTick{ config.opticalTick }
  , fBaselineCheckParams{ config.baselineCheck, fOpticalTick }
  , fLogCategory{ config.logCategory }
  {}


//------------------------------------------------------------------------------
sbn::opdet::OverlayPMTwaveformAlg::BaselineCheckParams_t::BaselineCheckParams_t
  (Config::BaselineCheck config, nanoseconds tick)
  : Config::BaselineCheck{ std::move(config) }
  , ticks{
      static_cast<std::size_t>
        (OverlayPMTwaveformAlg::ticksInInterval(length, tick))
    }
  {}


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::overlay(
  std::vector<InputWaveform_t> const& data,
  std::vector<InputWaveform_t> const& sim,
  std::optional<Interval_t> limitTo /* = std::nullopt */
) const -> OverlaidWaveforms
{
  // we need a signed type to appropriately deal with saturated ADC values
  static_assert(std::is_signed_v<raw::ADC_Count_t>);
  
  checkNoiselessBaseline(sim);

  OverlaidWaveforms res;
  res.waveforms.reserve(data.size());
  
  // sim waveforms grouped by channel and sorted by time:
  SimEntryLookup_t const simEntriesByChannel = makeSimEntryLookup(sim);

  // overlay all data waveforms, independently;
  // there is some extra simulation waveform lookup due to not going in sequence,
  // but in the big picture it should not have a relevant impact on performance
  for (InputWaveform_t const& d: data) {

    res.waveforms.push_back
      (overlayDataWaveform(d, simEntriesByChannel, limitTo));
    
  } // for data waveforms

  return res;

} // sbn::opdet::OverlayPMTwaveformAlg::overlay()


// -----------------------------------------------------------------------------
raw::OpDetWaveform sbn::opdet::OverlayPMTwaveformAlg::overlayDataWaveform(
  InputWaveform_t const& data, SimEntryLookup_t const& simEntriesByChannel,
  std::optional<Interval_t> limitTo
) const {
  
  assert(data.waveform);

  // start with a copy of the data waveform; the baseline is unused
  raw::OpDetWaveform resultW = *(data.waveform);
  Interval_t const dataInterval = waveformInterval(resultW);

  bool const doOverlay = !limitTo.has_value() || dataInterval.overlaps(*limitTo);

  if (!doOverlay) return resultW;

  raw::Channel_t const channel = resultW.ChannelNumber();
  auto const itChannelSimWaveforms = simEntriesByChannel.find(channel);
  auto const simRange = (itChannelSimWaveforms == simEntriesByChannel.end())
    ? ChannelSimEntries_t::SimEntryCRange_t{} // empty
    : itChannelSimWaveforms->second.overlappingWaveforms(dataInterval);
  
  for (auto const& [ simInputWaveform, simInterval ]: simRange) {
    
    assert(simInputWaveform.waveform);
    if (!simInputWaveform.baseline) {
      throw cet::exception{ "OverlayPMTwaveformAlg" }
        << "Waveform CH=" << simInputWaveform.waveform->ChannelNumber()
        << " TS=" << simInputWaveform.waveform->TimeStamp()
        << " was provided without baseline.\n";
    }
    
    raw::OpDetWaveform const& simW = *(simInputWaveform.waveform);

    assert(simW.ChannelNumber() == channel);
    assert(dataInterval.overlaps(simInterval));
    
    overlayWaveform(resultW, simW, *(simInputWaveform.baseline));

  } // for simulated waveforms in interval

  return resultW;

} // sbn::opdet::OverlayPMTwaveformAlg::overlayDataWaveform()


//------------------------------------------------------------------------------
void sbn::opdet::OverlayPMTwaveformAlg::overlayWaveform(
  raw::OpDetWaveform& dest,
  raw::OpDetWaveform const& contrib,
  icarus::WaveformBaseline const& baseline
) const {
  
  // we need a signed type to appropriately deal with saturated ADC values
  static_assert(std::is_signed_v<raw::ADC_Count_t>);
  
  Interval_t const destInt = waveformInterval(dest);
  Interval_t const contribInt = waveformInterval(contrib);
  Interval_t const overlapInt = destInt.intersection(contribInt);
  
  // indices for the copy
  std::size_t const iDestEnd = timeToSample(overlapInt.stop, destInt.start);
  std::size_t iDest = timeToSample(overlapInt.start, destInt.start);
  if (iDest >= iDestEnd) return; // overlap is smaller than 1 tick
  std::size_t iContrib = timeToSample(overlapInt.start, contribInt.start);
  
  // we round the baseline here, since the rest of the math is integral
  auto const simBaseline
    = static_cast<raw::ADC_Count_t>(std::round(baseline.baseline()));

  do {
    dest[iDest] = clampToADC(dest[iDest] + contrib[iContrib] - simBaseline);
    ++iContrib;
  } while (++iDest < iDestEnd);
  
} // sbn::opdet::OverlayPMTwaveformAlg::overlayWaveform()


//------------------------------------------------------------------------------
void sbn::opdet::OverlayPMTwaveformAlg::checkNoiselessBaseline
  (std::vector<InputWaveform_t> const& sim) const
{
  if (!fBaselineCheckParams.enabled()) return;
  if (sim.empty()) return;
  
  std::vector<std::string> failures;
  for (auto const [ waveform, baseline ]: sim) {
    assert(waveform);
    assert(baseline);
    std::string errorMsg
      = checkNoiselessBaseline(*waveform, baseline->baseline());
    if (!errorMsg.empty()) failures.emplace_back(std::move(errorMsg));
  } // for all waveforms
  
  if (failures.size() > sim.size() * fBaselineCheckParams.tolerance) {
    cet::exception e{ "OverlayPMTwaveformAlg" };
    e << "OverlayPMTwaveformAlg: " << failures.size() << "/" << sim.size()
      << " waveforms do not start with a constant baseline";
    auto const nPrinted = std::min<std::size_t>(failures.size(), 10U);
    if (nPrinted != failures.size()) e << "; first " << nPrinted;
    e << ":";
    for (std::size_t i = 0; i < nPrinted; ++i) e << "\n  " << failures[i];
    e << "\n";
    throw e;
  }
} // sbn::opdet::OverlayPMTwaveformAlg::checkNoiselessBaseline()


//------------------------------------------------------------------------------
std::string sbn::opdet::OverlayPMTwaveformAlg::checkNoiselessBaseline
  (raw::OpDetWaveform const& waveform, float baseline) const
{
  std::size_t const nToCheck
    = std::min(fBaselineCheckParams.ticks, waveform.Waveform().size());
  if (nToCheck == 0U) return "";
  
  raw::ADC_Count_t const roundBaseline = std::lround(baseline);

  if (
    !std::all_of(waveform.cbegin(), waveform.cbegin() + nToCheck,
      [roundBaseline](raw::ADC_Count_t sample){ return sample == roundBaseline; })
  ) {
    return "CH=" + std::to_string(waveform.ChannelNumber())
      + " T=" + std::to_string(waveform.TimeStamp())
      + " (baseline: " + std::to_string(roundBaseline) + ")";
  }
  
  return std::string{}; // empty string: success
} // sbn::opdet::OverlayPMTwaveformAlg::checkNoiselessBaseline()


//------------------------------------------------------------------------------
std::ptrdiff_t sbn::opdet::OverlayPMTwaveformAlg::ticksInInterval
  (nanoseconds interval) const
{
  return ticksInInterval(interval, fOpticalTick);
}


//------------------------------------------------------------------------------
std::ptrdiff_t sbn::opdet::OverlayPMTwaveformAlg::ticksInInterval
  (nanoseconds interval, nanoseconds tick)
{
  return static_cast<std::ptrdiff_t>(std::llround(interval / tick));
}


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::makeSimEntryLookup
  (std::vector<InputWaveform_t> const& waveforms) const -> SimEntryLookup_t
{
  SimEntryLookup_t map;
  
  // this was intended to derive from raw::OrderOpDetWaveformByChannelAndTime,
  // but eventually we sort by end time
  struct EarlierStopThan {
    double tickSize;
    
    static bool cmp (raw::OpDetWaveform const& A, raw::OpDetWaveform const& B) noexcept
      {
        // C++20: implement with <=> operator
        if (A.ChannelNumber() != B.ChannelNumber())
          return A.ChannelNumber() < B.ChannelNumber();
        return A.TimeStamp() < B.TimeStamp();
      }
    
    static raw::OpDetWaveform const& waveformOf
      (InputWaveform_t const& input) noexcept
      { return *(input.waveform); }
    
    bool operator() (InputWaveform_t const& a, InputWaveform_t const& b) const
      { return cmp(waveformOf(a), waveformOf(b)); }
  };
  
  // optical tick converted in a plain number with same unit as electronics_time
  EarlierStopThan const earlierWaveform
    { fOpticalTick.quantity().convertInto<electronics_time::quantity_t>().value() };
  
  ChannelSimEntries_t* currentChannel = nullptr;
  
  for (InputWaveform_t const& input: sorted(waveforms, earlierWaveform)) {
    assert(input.waveform);
    
    raw::Channel_t const channel = input.waveform->ChannelNumber();
    if (!currentChannel || (currentChannel->channel() != channel))
      currentChannel = &(map[channel]);
    
    currentChannel->append({ input, waveformInterval(*(input.waveform)) });
  } // for
  
  return map;
  
} // sbn::opdet::OverlayPMTwaveformAlg::makeSimEntryLookup()


//------------------------------------------------------------------------------
std::ptrdiff_t sbn::opdet::OverlayPMTwaveformAlg::timeToSample
  (electronics_time t, electronics_time start /* = {} */) const
{
  return ticksInInterval(t - start);
}


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::snappedStartTime
  (raw::OpDetWaveform const& waveform) const -> electronics_time
{
  using util::quantities::points::microsecond;

  electronics_time const start{ microsecond{ waveform.TimeStamp() } };
  
  return electronics_time{} + timeToSample(start) * fOpticalTick ;
} // sbn::opdet::OverlayPMTwaveformAlg::snappedStartTime()


//------------------------------------------------------------------------------
auto sbn::opdet::OverlayPMTwaveformAlg::waveformInterval
  (raw::OpDetWaveform const& waveform) const -> Interval_t
{
  electronics_time const start = snappedStartTime(waveform);
  electronics_time const stop = start + fOpticalTick * waveform.Waveform().size();
  return Interval_t{ start, stop };
} // sbn::opdet::OverlayPMTwaveformAlg::waveformInterval()


//------------------------------------------------------------------------------
raw::ADC_Count_t sbn::opdet::OverlayPMTwaveformAlg::clampToADC
  (raw::ADC_Count_t value)
{
  return std::clamp<raw::ADC_Count_t>(value, 0, 16383);
}


//------------------------------------------------------------------------------
