/**
 * @file   TimeIntervalConfig_test.cc
 * @brief  Unit test for `icarus::ns::fhicl::TimeIntervalConfig`.
 * @date   August 18, 2023
 * @author petrillo@slac.stanford.edu
 *
 * Usage: just run the executable.
 */

// Boost test libraries; defining this symbol tells boost somehow to generate
// a main() function
#define BOOST_TEST_MODULE TimeIntervalConfigTest
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_TEST()

// ICARUS libraries
#include "icarusalg/Utilities/TimeIntervalConfig.h"

// LArSoft libraries
#include "lardataalg/Utilities/quantities/spacetime.h"
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // electronics_time

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/types/Table.h"
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <optional>


// -----------------------------------------------------------------------------
void TimeIntervalTable_doc2_test() {
  /*
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
  
  class MyAlgorithm {
  
      public:
    
    using electronics_time = detinfo::timescales::electronics_time;
    
    struct Config {
      
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
      
      icarus::ns::fhicl::TimeIntervalTable<electronics_time> Interval{
        Name{ "Interval" },
        Comment{ "specify the selection time interval" }
        };
      
    };
    
    using Parameters = fhicl::Table<Config>;
    
    MyAlgorithm(Parameters const& params)
      : fInterval{ params().Interval() }
      {
        mf::LogInfo{ "MyAlgorithm" } << "Time interval: " << fInterval;
      }
    
      private:
    icarus::ns::util::TimeInterval<electronics_time> fInterval;
    
      public:
    icarus::ns::util::TimeInterval<electronics_time> const& interval() const
      { return fInterval; }
    
  };
  
  auto const config = fhicl::ParameterSet::make(R"(
myalgorithm: {
  Interval: { Start: "-5 us"  Duration: "+20 us" }
}
)");

  // ---------------------------------------------------------------------------
  using namespace util::quantities::time_literals;

  MyAlgorithm const testAlg{ config.get<fhicl::ParameterSet>("myalgorithm") };
  
  auto const& interval = testAlg.interval();
  BOOST_TEST(interval.start.value() == -5.0);
  BOOST_TEST(interval.stop.value() == +15.0);
  
} // TimeIntervalTable_doc2_test()


// -----------------------------------------------------------------------------
void TimeIntervalOptionalTable_doc2_test() {
  /*
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
   *     : fInterval{ params().Interval().value_or(DefaultInterval) }
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
  
  using namespace util::quantities::time_literals; // for `200_ns` etc.
  
  static constexpr
  icarus::ns::util::TimeInterval<util::quantities::points::nanosecond>
    DefaultInterval{ -100_ns, +200_ns};
  
  class MyAlgorithm {
  
      public:
    using nanosecond = util::quantities::points::nanosecond;
    
    struct Config {
      
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
      
      icarus::ns::fhicl::TimeIntervalOptionalTable<nanosecond> Interval{
        Name{ "Interval" },
        Comment{ "override the selection time interval" }
        };
      
    };
    
    using Parameters = fhicl::Table<Config>;
    
    MyAlgorithm(Parameters const& params)
      : fInterval{ params().Interval().value_or(DefaultInterval) }
      {
         mf::LogInfo{ "MyAlgorithm" }
           << "Time interval: " << fInterval;
      }
    
      private:
    icarus::ns::util::TimeInterval<nanosecond> fInterval;
    
      public:
    icarus::ns::util::TimeInterval<nanosecond> const& interval() const
      { return fInterval; }
    
  };
  
  // inline constexpr icarus::ns::util::TimeInterval<util::quantities::points::nanosecond>
  // MyAlgorithm::DefaultInterval;

  auto const config1 = fhicl::ParameterSet::make(R"(
myalgorithm: {
  Interval: { Start: "-5 us"  Duration: "+20 us" }
}
)");
  
  auto const config2 = fhicl::ParameterSet::make(R"(
myalgorithm: {
}
)");
  
  // ---------------------------------------------------------------------------
  MyAlgorithm const testAlg1
    { config1.get<fhicl::ParameterSet>("myalgorithm") };
  
  auto const& interval1 = testAlg1.interval();
  BOOST_TEST(interval1.start.value() ==  -5'000.0);
  BOOST_TEST(interval1.stop.value()  == +15'000.0);
  
  MyAlgorithm const testAlg2
    { config2.get<fhicl::ParameterSet>("myalgorithm") };
  
  auto const& interval2 = testAlg2.interval();
  BOOST_TEST(interval2.start.value() == -100.0);
  BOOST_TEST(interval2.stop.value()  == +200.0);
  
} // TimeIntervalOptionalTable_doc2_test()


// -----------------------------------------------------------------------------
void TimeIntervalConfig_doc_test() {
  /*
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   */
  
  auto const config = fhicl::ParameterSet::make(R"(
intervalA: {  Start: "-5 us"  Duration: "15 us"  }  # [ -5 ; +10 ] us
intervalB: {  Start: "-5 us"  End: "10 us"  }       # [ -5 ; +10 ] us
intervalC: {  Duration: "15 us"  End: "10 us"  }    # [ -5 ; +10 ] us
intervalD: {  Duration: "200 ns"  }                 # [  0 ; +0.2 ] us
intervalE: {  End: "200 ns"  }                      # [  0 ; +0.2 ] us
intervalF: {  }                                     # [  0 ; 0 ]

# this (valid FHiCL) would not parse with `makeTimeInterval()` helpers:
intervalG: {  Start: "-5 us"  Duration: "15 us"  End: "10 us"  }
)");
  
  // ---------------------------------------------------------------------------
  using microsecond = util::quantities::points::microsecond;
  icarus::ns::util::TimeInterval<microsecond> interval;

  std::optional<fhicl::Table<icarus::ns::fhicl::TimeIntervalConfig<microsecond>>>
  configTable;

  configTable.emplace(config.get<fhicl::ParameterSet>("intervalA"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() ==  -5.0);
  BOOST_TEST(interval.stop.value()  == +10.0);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalB"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() ==  -5.0);
  BOOST_TEST(interval.stop.value()  == +10.0);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalC"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() ==  -5.0);
  BOOST_TEST(interval.stop.value()  == +10.0);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalD"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() == 0.0);
  BOOST_TEST(interval.stop.value()  == 0.2);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalE"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() == 0.0);
  BOOST_TEST(interval.stop.value()  == 0.2);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalF"));
  interval = makeTimeInterval((*configTable)());
  BOOST_TEST(interval.start.value() == 0.0);
  BOOST_TEST(interval.stop.value()  == 0.0);
  
  configTable.emplace(config.get<fhicl::ParameterSet>("intervalG"));
  icarus::ns::fhicl::TimeIntervalConfig const intervalConfig = (*configTable)();
  BOOST_CHECK_THROW(makeTimeInterval(intervalConfig), cet::exception);
  
} // TimeIntervalConfig_doc_test()


// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(TimeIntervalTable_tests) {
  
  TimeIntervalTable_doc2_test();
  
} // BOOST_AUTO_TEST_CASE(TimeIntervalTable_tests)

BOOST_AUTO_TEST_CASE(TimeIntervalOptionalTable_tests) {
  
  TimeIntervalOptionalTable_doc2_test();
  
} // BOOST_AUTO_TEST_CASE(TimeIntervalOptionalTable_tests)


BOOST_AUTO_TEST_CASE(TimeIntervalConfig_tests) {
  
  TimeIntervalConfig_doc_test();
  
} // BOOST_AUTO_TEST_CASE(TimeIntervalConfig_tests)


// -----------------------------------------------------------------------------
