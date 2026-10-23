/**
 * @file   icarusalg/Utilities/TimeInterval.h
 * @brief  Simple time interval object.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 4, 2023
 * 
 * This library is header only.
 */

#ifndef ICARUSALG_UTILITIES_TIMEINTERVAL_H
#define ICARUSALG_UTILITIES_TIMEINTERVAL_H


// C/C++ standard libraries
#include <ostream>
#include <utility> // std::declval()
#include <cstddef> // std::size_t


//------------------------------------------------------------------------------
namespace icarus::ns::util {
  template <typename Time> struct TimeInterval;
  
  template <typename Time>
  std::ostream& operator<< (std::ostream& out, TimeInterval<Time> const& time);
  
  /// Returns a new `interval` shifted by `shift` in the future.
  template <typename TimeI, typename TimeS>
  constexpr auto operator+ (TimeInterval<TimeI> const& interval, TimeS shift);

  /// Returns a new `interval` shifted by `shift` in the future.
  template <typename TimeI, typename TimeS>
  constexpr auto operator+ (TimeS shift, TimeInterval<TimeI> const& interval);
  
  /// Returns a new `interval` shifted by `shift` in the past.
  template <typename TimeI, typename TimeS>
  constexpr auto operator- (TimeInterval<TimeI> const& interval, TimeS shift);
  
  template <std::size_t I, typename Time> Time get(TimeInterval<Time> const&);
  template <std::size_t I, typename Time> Time& get(TimeInterval<Time>&);
  
} // namespace icarus::ns::util


//------------------------------------------------------------------------------
/**
 * @brief Simple time interval, a `start` and a `stop` (of type `T`).
 * @tparam Time type of time for the interval
 * 
 * This simple class is mostly a data structure for use with 
 * `icarus::ns::fhicl::TimeIntervalConfig` to read a time interval from
 * configuration.
 * 
 * It is designed with some care to allow custom data types like LArSoft time
 * quantities, e.g. `TimeInterval<detinfo::timescales::electronics_time>`.
 */
template <typename Time>
struct icarus::ns::util::TimeInterval {
  
  using TimeInterval_t = TimeInterval<Time>; ///< This type.
  
  using Time_t = Time; ///< Type of time used.
  
  /// Type of time difference.
  using TimeDiff_t = decltype(std::declval<Time_t>() - std::declval<Time_t>());
  
  
  Time_t start;         ///< Start time of the interval (included).
  Time_t stop{ start }; ///< End time of the interval (excluded).
  
  
  // --- BEGIN -- Constructors -------------------------------------------------
  /// @name Constructors
  /// @{
  
  /// Constructor: an empty interval.
  constexpr TimeInterval() = default;
  
  /// Constructor: sets an empty interval starting at `start` time.
  constexpr TimeInterval(Time_t start): TimeInterval{ start, start } {}
  
  /// Constructor: sets start and stop time.
  constexpr TimeInterval(Time_t start, Time_t stop)
    : start{ start }, stop{ stop } {}
  
  /// Constructor: copies from other time interval types.
  template <typename OtherType>
  constexpr TimeInterval(TimeInterval<OtherType> const& other)
    : TimeInterval{ other.start, other.stop } {}
  
  /// @}
  // --- END ---- Constructors -------------------------------------------------
  
  
  // --- BEGIN -- Query --------------------------------------------------------
  /// @name Query
  /// @{
  
  /// Returns whether the interval is empty.
  constexpr bool empty() const noexcept { return start >= stop; }
  
  /// Returns the total length/duration of the interval.
  constexpr TimeDiff_t duration() const noexcept { return stop - start; }
  
  /// @}
  // --- END ---- Query --------------------------------------------------------
  
  
  // --- BEGIN -- Operations ---------------------------------------------------
  /// @name Simple algorithms
  /// @{
  
  /// Returns whether `t` is between `start` (included) and `stop` (excluded).
  constexpr bool contains(Time_t t) const noexcept
    { return (t >= start) && (t < stop); }
  
  /// Returns whether this interval is earlier than `other` (even adjacent).
  /// @note Undefined if either interval is empty.
  template <typename OtherTime>
  constexpr bool before(TimeInterval<OtherTime> const& other) const noexcept
    { return stop <= other.start; }
  
  /// Returns whether this interval is later than `other` (even adjacent).
  /// @note Undefined if either interval is empty.
  template <typename OtherTime>
  constexpr bool after(TimeInterval<OtherTime> const& other) const noexcept
    { return other.before(*this); }
  
  /// Returns whether this interval overlaps `other` (adjacent one doesn't).
  template <typename OtherTime>
  constexpr bool overlaps(TimeInterval<OtherTime> const& other) const noexcept
    { return !empty() && !other.empty() && !before(other) && !after(other); }
  
  /// Returns a copy of this interval intersected with `other`.
  template <typename OtherTime>
  constexpr TimeInterval_t intersection(TimeInterval<OtherTime> other) const
    { return other.intersect(*this); }
  
  /// @}
  // --- END ---- Operations ---------------------------------------------------
  
  
  // --- BEGIN -- Modifiers ----------------------------------------------------
  /// @name Modifiers
  /// @{
  
  /// Adds the specified `amount` of time to the interval ends, if not empty.
  template <typename OtherTime>
  TimeInterval_t& shift(OtherTime amount)
    { if (!empty()) { start += amount; stop += amount; } return *this; }
  
  /// Reduces this interval to its intersection with `other`.
  template <typename OtherTime>
  TimeInterval_t& intersect(TimeInterval<OtherTime> const& other)
    {
      if (start < other.start) start = other.start;
      if (stop > other.stop) stop = other.stop;
      return *this;
    }
  
  /// Reduces this interval to its extension with `other`.
  template <typename OtherTime>
  TimeInterval_t& extend(TimeInterval<OtherTime> const& other)
    {
      if (empty()) *this = other;
      else if (!other.empty()) {
        if (other.start < start) start = other.start;
        if (other.stop > stop) stop = other.stop;
      }
      return *this;
    }
  
  /// Adds the specified `amount` of time to the interval ends, if not empty.
  template <typename OtherTime>
  TimeInterval_t& operator += (OtherTime amount)
    { return shift(amount); }
  
  /// Subtracts a specified `amount` of time to the interval ends, if not empty.
  template <typename OtherTime>
  TimeInterval_t& operator -= (OtherTime amount)
    { return shift(-amount); }
  
  /// @}
  // --- END ---- Modifiers ----------------------------------------------------
  
}; // icarus::ns::util::TimeInterval


//--------------------------------------------------------------------------
//---  Template implementation
//--------------------------------------------------------------------------
template <typename TimeI, typename TimeS>
constexpr auto icarus::ns::util::operator+
  (icarus::ns::util::TimeInterval<TimeI> const& interval, TimeS shift)
{
  return TimeInterval{ interval.start + shift, interval.stop + shift };
}

template <typename TimeI, typename TimeS>
constexpr auto icarus::ns::util::operator+
  (TimeS shift, icarus::ns::util::TimeInterval<TimeI> const& interval)
{
  // note that Interval<> + Point<> is not supported, Point<>+Interval<> is
  return TimeInterval{ shift + interval.start, shift + interval.stop };
}


template <typename TimeI, typename TimeS>
constexpr auto icarus::ns::util::operator-
  (icarus::ns::util::TimeInterval<TimeI> const& interval, TimeS shift)
{
  return TimeInterval{ interval.start - shift, interval.stop - shift };
}


//--------------------------------------------------------------------------
template <typename Time>
std::ostream& icarus::ns::util::operator<<
  (std::ostream& out, TimeInterval<Time> const& time)
{
  if (time.empty()) out << "[ empty ]";
  else out << "[ " << time.start << " ; " << time.stop << "]";
  return out;
}


//--------------------------------------------------------------------------
template <std::size_t I, typename Time>
Time icarus::ns::util::get(TimeInterval<Time> const& interval) {
  // this is written for when we'll use C++20
  // (C++17 does not find this `get()` with argument-based lookup)
  static_assert(I == 0 || I == 1, "Invalid index for get(TimeInterval)");
  if constexpr(I == 0) return interval.start;
  if constexpr(I == 1) return interval.stop;
} // icarus::ns::util::get(TimeInterval)


template <std::size_t I, typename Time>
Time& icarus::ns::util::get(TimeInterval<Time>& interval) {
  // this is written for when we'll use C++20
  // (C++17 does not find this `get()` with argument-based lookup)
  static_assert(I == 0 || I == 1, "Invalid index for get(TimeInterval)");
  if constexpr(I == 0) return interval.start;
  if constexpr(I == 1) return interval.stop;
} // icarus::ns::util::get(TimeInterval)


//--------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_TIMEINTERVAL_H
