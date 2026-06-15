/**
 * @file   icarusalg/Utilities/TimeIntervalConfig.h
 * @brief  Helper for specifying a time interval parameter in FHiCL.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 4, 2023
 * 
 * This library is header only.
 */

#ifndef ICARUSALG_UTILITIES_TIMEINTERVALCONFIG_H
#define ICARUSALG_UTILITIES_TIMEINTERVALCONFIG_H


// ICARUS libraries
#include "icarusalg/Utilities/TimeInterval.h"
#include "lardataalg/Utilities/intervals_fhicl.h" // convenience
#include "lardataalg/Utilities/quantities_fhicl.h" // convenience

// framework libraries
#include "fhiclcpp/types/OptionalTableAs.h"
#include "fhiclcpp/types/TableAs.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"

// C/C++ standard libraries
#include <optional>


//--------------------------------------------------------------------------
namespace icarus::ns::fhicl {
  
  /**
   * @brief FHiCL configuration for a `icarus::ns::util::TimeInterval` object.
   * @tparam Time type of time the interval object uses
   * @see `icarus::ns::fhicl::TimeIntervalTable`,
   *      `icarus::ns::fhicl::TimeIntervalOptionalTable`
   * 
   * Objects from this class can be included in the validated FHiCL
   * configuration objects. See `icarus::ns::fhicl::TimeIntervalTable`
   * and `icarus::ns::fhicl::TimeIntervalOptionalTable` for usage examples.
   */
  template <typename Time> struct TimeIntervalConfig;
  
  /**
   * @brief Extracts a `icarus::ns::util::TimeInterval` value from a FHiCL
   *        configuration.
   * @tparam Time type of time the interval object uses
   * @param config the FHiCL configuration to read the interval from
   * @return an interval object
   * @see `icarus::ns::fhicl::TimeIntervalTable`
   * 
   * See `icarus::ns::fhicl::TimeIntervalTable` for an usage example.
   */
  template <typename Time>
  icarus::ns::util::TimeInterval<Time> makeTimeInterval
    (TimeIntervalConfig<Time> const& config);
  
  /**
   * @brief Extracts a `icarus::ns::util::TimeInterval` value from an optional
   *        FHiCL configuration.
   * @tparam Time type of time the interval object uses
   * @param config the optional FHiCL configuration to read the interval from
   * @return an optional interval object, `std::nullopt` if not in `config`
   * @see `icarus::ns::fhicl::TimeIntervalOptionalTable`
   * 
   * This object can also be used as a way to supply a default value to a
   * `icarus::ns::util::TimeInterval` parameter.
   * See `icarus::ns::fhicl::TimeIntervalOptionalTable` for an usage example.
   */
  template <typename Time>
  std::optional<icarus::ns::util::TimeInterval<Time>> makeTimeInterval
    (std::optional<TimeIntervalConfig<Time>> const& config);
  
  /**
   * @brief FHiCL configuration object for specification of a (time) interval.
   * @tparam Time the type of the time point being read by the configuration
   * @see `icarus::ns::fhicl::TimeIntervalConfig`
   * 
   * This is a handy alias for a FHiCL configuration with a
   * `icarus::ns::fhicl::TimeIntervalConfig` entry.
   * Note that FHiCL tables do not support default values, and to get something
   * similar to it `TimeIntervalOptionalTable` can be used instead.
   * 
   * Use it like in the FHiCL configuration object of an algorithm or module:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * using electronics_time = detinfo::timescales::electronics_time;
   * 
   * struct Config {
   *   
   *   using Name = fhicl::Name;
   *   using Comment = fhicl::Comment;
   *   
   *   TimeIntervalTable<electronics_time> Interval{
   *     Name{ "Interval" },
   *     Comment{ "specify the selection time interval" }
   *     };
   *   
   *   // ...
   *   
   * };
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * It can be read for example in the constructor of a class (e.g. a module):
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * class MyAlgorithm {
   * 
   *     public:
   *   
   *   using electronics_time = detinfo::timescales::electronics_time;
   *   
   *   struct Config {
   *     
   *     using Name = fhicl::Name;
   *     using Comment = fhicl::Comment;
   *     
   *     icarus::ns::fhicl::TimeIntervalTable<electronics_time> Interval{
   *       Name{ "Interval" },
   *       Comment{ "specify the selection time interval" }
   *       };
   *     
   *   };
   *   
   *   using Parameters = fhicl::Table<Config>;
   *   
   *   MyAlgorithm(Parameters const& params)
   *     : fInterval{ params().Interval() }
   *     {
   *       mf::LogInfo{ "MyAlgorithm" } << "Time interval: " << fInterval;
   *     }
   *   
   *     private:
   *   icarus::ns::util::TimeInterval<electronics_time> fInterval;
   *   
   * };
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * with a FHiCL configuration like:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * myalgorithm: {
   *   Interval: { Start: "-5 us"  Duration: "+20 us" }
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   */
  template <typename Time>
  using TimeIntervalTable = ::fhicl::TableAs
    <icarus::ns::util::TimeInterval<Time>, TimeIntervalConfig<Time>>;
  
  /**
   * @brief FHiCL optional configuration object for specification of a (time)
   *        interval.
   * @tparam Time the type of the time point being read by the configuration
   * @see `icarus::ns::fhicl::TimeIntervalConfig`
   * 
   * This is a handy alias for a FHiCL configuration with an optional
   * `icarus::ns::fhicl::TimeIntervalConfig` entry.
   * Note that since FHiCL tables do not support default values, this is the
   * way to have one.
   * 
   * Use it like in the FHiCL configuration object of an algorithm or module:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * using nanosecond = util::quantities::points::nanosecond;
   * 
   * struct Config {
   *   
   *   using Name = fhicl::Name;
   *   using Comment = fhicl::Comment;
   *   
   *   TimeIntervalOptionalTable<nanosecond> Interval{
   *     Name{ "Interval" },
   *     Comment{ "override the selection time interval" }
   *     };
   *   
   *   // ...
   *   
   * };
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * It can be read for example in the constructor of a class (e.g. a module):
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * using namespace util::quantities::time_literals; // for `200_ns` etc.
   * 
   * class MyAlgorithm {
   * 
   *     public:
   *   using nanosecond = util::quantities::points::nanosecond;
   *   
   *   struct Config {
   *     
   *     using Name = fhicl::Name;
   *     using Comment = fhicl::Comment;
   *     
   *     icarus::ns::fhicl::TimeIntervalOptionalTable<nanosecond> Interval{
   *       Name{ "Interval" },
   *       Comment{ "override the selection time interval" }
   *       };
   *     
   *   };
   *   
   *   using Parameters = fhicl::Table<Config>;
   *   
   *   MyAlgorithm(Parameters const& params)
   *     : fInterval
   *       {
   *         icarus::ns::fhicl::makeTimeInterval(params().Interval())
   *           .value_or(DefaultInterval)
   *       }
   *     {
   *       mf::LogInfo{ "MyAlgorithm" } << "Time interval: " << fInterval;
   *     }
   *   
   *     private:
   *   icarus::ns::util::TimeInterval<nanosecond> fInterval;
   *   
   *   static constexpr icarus::ns::util::TimeInterval<nanosecond>
   *     DefaultInterval{ -100_ns, +200_ns};
   * };
   * 
   * inline constexpr icarus::ns::util::TimeInterval<util::quantities::points::nanosecond>
   *   MyAlgorithm::DefaultInterval{ -100_ns, +200_ns};
   * 
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * with a FHiCL configuration like:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * myalgorithm: {
   *   Interval: { Start: "-5 us"  Duration: "+20 us" }
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * or it can be omitted altogether:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * myalgorithm: {
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * (_not_ the same as specifying `Interval: {}`, which will yield an empty
   * `[ 0, 0 ]` interval).
   */
  template <typename Time>
  using TimeIntervalOptionalTable = ::fhicl::OptionalTableAs
    <icarus::ns::util::TimeInterval<Time>, TimeIntervalConfig<Time>>;
  
} // namespace icarus::ns::fhicl


// -----------------------------------------------------------------------------
/**
 * @brief FHiCL configuration object for specification of a (time) interval.
 * @tparam Time the type of the time point being read by the configuration
 * @see `icarus::ns::fhicl::TimeIntervalTable`,
 *      `icarus::ns::fhicl::TimeIntervalOptionalTable`
 * 
 * See `icarus::ns::fhicl::TimeIntervalTable` and
 * `icarus::ns::fhicl::TimeIntervalOptionalTable` for complete example usages.
 * 
 * Examples of configuration:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * intervalA: {  Start: "-5 us"  Duration: "15 us"  }  # [ -5 ; +10 ] us
 * intervalB: {  Start: "-5 us"  End: "10 us"  }       # [ -5 ; +10 ] us
 * intervalC: {  Duration: "15 us"  End: "10 us"  }    # [ -5 ; +10 ] us
 * intervalD: {  Duration: "200 ns"  }                 # [  0 ; +0.2 ] us
 * intervalE: {  End: "200 ns"  }                      # [  0 ; +0.2 ] us
 * intervalF: {  }                                     # [  0 ; 0 ]
 * 
 * # this (valid FHiCL) would not parse with `makeTimeInterval()` helpers:
 * # intervalG: {  Start: "-5 us"  Duration: "15 us"  End: "10 us"  }
 * 
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
template <typename Time> 
struct icarus::ns::fhicl::TimeIntervalConfig {
  
  using Name = ::fhicl::Name;
  using Comment = ::fhicl::Comment;
  
  using TimeQuantity_t = Time; ///< Type of time being read.
  
  /// Type of time difference.
  using Duration_t
    = decltype(std::declval<TimeQuantity_t>() - std::declval<TimeQuantity_t>());
  
  
  ::fhicl::OptionalAtom<TimeQuantity_t> Start{
    Name{ "Start" },
    Comment{ "start time [default: since forever]" }
    };
  
  ::fhicl::OptionalAtom<TimeQuantity_t> End{
    Name{ "End" },
    Comment{ "end time [default: to forever]" }
    };
  
  ::fhicl::OptionalAtom<Duration_t> Duration{
    Name{ "Duration" },
    Comment{ "interval duration [default: forever]" }
    };
  
}; // icarus::ns::fhicl::TimeIntervalConfig


//--------------------------------------------------------------------------
// --- template implementation
//--------------------------------------------------------------------------
template <typename Time>
icarus::ns::util::TimeInterval<Time> icarus::ns::fhicl::makeTimeInterval
  (TimeIntervalConfig<Time> const& config)
{
  // start with default: -0 / +0 (empty):
  icarus::ns::util::TimeInterval<Time> interval;
  
  if (config.Start()) {
    interval.start = *(config.Start());
    if (config.End()) {
      interval.stop = *(config.End());
      if (config.Duration()) {
        throw cet::exception{ "makeTimeInterval" }
          << "Only up to two among '" << config.Start.name() << "', '"
          << config.End.name() << "' and '" << config.Duration.name()
          << "' parameters must be specified for a time interval.\n";
      }
    }
    else if (config.Duration()) {
      interval.stop = interval.start + *(config.Duration());
    }
  }
  else if (config.End()) {
    interval.stop = *(config.End());
    if (config.Duration())
      interval.start = interval.stop - *(config.Duration());
  }
  else if (config.Duration()) {
    interval.stop = interval.start + *(config.Duration());
  }
  
  return interval;
} // icarus::ns::fhicl::makeTimeInterval()


//--------------------------------------------------------------------------
template <typename Time>
std::optional<icarus::ns::util::TimeInterval<Time>>
icarus::ns::fhicl::makeTimeInterval
  (std::optional<TimeIntervalConfig<Time>> const& config)
  { return config? std::optional{ makeTimeInterval(*config) }: std::nullopt; }


//--------------------------------------------------------------------------
namespace icarus::ns::fhicl {
  // this is for FHiCL to directly convert a configuration object to interval
  template <typename Time>
  icarus::ns::util::TimeInterval<Time> convert
    (TimeIntervalConfig<Time> const& config)
    { return makeTimeInterval(config); }
}

//--------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_TIMEINTERVALCONFIG_H
