/**
 * @file   icarusalg/Utilities/OpDetWaveformUtils.h
 * @brief  Simple utilities for `raw::OpDetWaveform` objects.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   March 19, 2026, 2026
 * 
 * This library is currently header only.
 */

#ifndef ICARUSALG_UTILITIES_OPDETWAVEFORMUTILS_H
#define ICARUSALG_UTILITIES_OPDETWAVEFORMUTILS_H


// LArSoft libraries
#include "lardataobj/RawData/OpDetWaveform.h"


// -----------------------------------------------------------------------------
// forward declarations
namespace raw { struct OrderOpDetWaveformByChannelAndTime; }


// -----------------------------------------------------------------------------
/**
 * @brief Functor for comparing and sorting waveforms by channel and then time.
 * 
 * This enables the "canonical" order we expect waveforms to be stored with.
 * 
 * Example of usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * template <typename WaveformColl>
 * void sortWaveforms(WaveformColl& waveforms) {
 *   using std::begin, std::end;
 *   std::sort(begin(waveforms), end(waveforms),
 *     raw::OrderOpDetWaveformByChannelAndTime{});
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * will sort in place the `waveforms`, grouping them by channel and, within each
 * group, by time (from lowest to highest in both cases).
 * 
 */
struct raw::OrderOpDetWaveformByChannelAndTime {
  
  static bool cmp (raw::OpDetWaveform const& A, raw::OpDetWaveform const& B) noexcept
    {
      // C++20: implement with <=> operator
      if (A.ChannelNumber() != B.ChannelNumber())
        return A.ChannelNumber() < B.ChannelNumber();
      return A.TimeStamp() < B.TimeStamp();
    }
  
  bool operator()
    (raw::OpDetWaveform const& A, raw::OpDetWaveform const& B) const noexcept
    { return cmp(A, B); }
  
}; // raw::OrderWaveformByChannelAndTime


// -----------------------------------------------------------------------------

#endif // ICARUSALG_UTILITIES_OPDETWAVEFORMUTILS_H
