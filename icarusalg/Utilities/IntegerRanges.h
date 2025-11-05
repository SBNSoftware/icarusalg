/**
 * @file   icarusalg/Utilities/IntegerRanges.h
 * @brief  Class compacting a list of integers.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   May 18, 2021
 * 
 * This is a header-only, pure standard C++ library.
 */


#ifndef ICARUSALG_UTILITIES_INTEGERRANGES_H
#define ICARUSALG_UTILITIES_INTEGERRANGES_H

// C/C++ standard libraries
#include <ostream>
#include <vector>
#include <initializer_list>
#include <numeric> // std::accumulate()
#include <stdexcept> // std::runtime_error
#include <type_traits> // std::is_integral_v


// -----------------------------------------------------------------------------
namespace icarus {
  
  // ---------------------------------------------------------------------------
  namespace details {
    
    template <typename T = int> class IntegerRangesBase;
    
    template <typename T>
    std::ostream& operator<< (
      std::ostream& out,
      typename IntegerRangesBase<T>::Data_t const& range
      );
    
  } // namespace details
  // ---------------------------------------------------------------------------
  
  
  template <typename T = int, bool CheckGrowing = false> class IntegerRanges;
  
  template <bool CheckGrowing = true , typename Coll>
  IntegerRanges<typename Coll::value_type, CheckGrowing> makeIntegerRanges
    (Coll const& coll);


  template <typename T, bool CheckGrowing>
  std::ostream& operator<<
    (std::ostream& out, IntegerRanges<T, CheckGrowing> const& ranges);
  
  template <typename T, bool CheckGrowing>
  std::ostream& operator<<
    (std::ostream& out, typename IntegerRanges<T, CheckGrowing>::Range_t const& r);
} // namespace icarus


// -----------------------------------------------------------------------------
/**
 * @brief A sequence of contiguous ranges of integral numbers.
 * @tparam T type of the integral numbers
 * @tparam CheckGrowing if `true`, checks will be performed on construction
 * 
 * This class parses a sequence in input grouping the consecutive elements.
 * The current interface is very simple, allowing only for query of groups
 * ("ranges") and printing to a stream.
 * The input is required and assumed to be a monotonously growing sequence,
 * with the exception that duplicate consecutive entries are allowed
 * (and ignored).
 * 
 * Each range is stored as a semi-open interval: [ _lower_, _upper_ [.
 */
template <typename T /* = int */>
class icarus::details::IntegerRangesBase {
  static_assert
    (std::is_integral_v<T>, "IntegerRanges only support integral types.");
  
    public:
  using Data_t = T; ///< Type of data for the range set.
  
  struct Range_t {
    
    Data_t lower {};
    Data_t upper {};
    
    constexpr Range_t() noexcept = default;
    constexpr Range_t(Data_t lower, Data_t upper) noexcept;
    
    constexpr bool empty() const noexcept;
    constexpr std::size_t size() const noexcept;
    constexpr bool isOne() const noexcept;
    constexpr bool isTwo() const noexcept;
    
    void dump(
      std::ostream& out,
      std::string const& sep, std::string const& simpleSep
      ) const;
    void dump(std::ostream& out, std::string const& sep = "--") const;
    
  }; // struct Range_t
  
  
  /// Removes all the entries and makes the set as default-constructed.
  void clear() noexcept;
  
  
  // --- BEGIN -- Queries ------------------------------------------------------
  /// @name Queries
  /// @{
  
  /// Returns whether there is any element in the range set.
  bool empty() const noexcept;
  
  /// Returns the number of elements in the ranges (gaps excluded).
  std::size_t size() const noexcept;
  
  /// Returns the number of non-contiguous ranges in the set.
  std::size_t nRanges() const noexcept;
  
  /// Returns an iterable object with all sorted ranges as elements.
  decltype(auto) ranges() const noexcept;
  
  /// @}
  // --- END ---- Queries ------------------------------------------------------
  
  
  /// Prints the range into the specified stream.
  /// @param out the stream to print into
  /// @param sep separator between ranges
  /// @param inRangeSep separator between lower and higher limit of each range
  void dump(std::ostream& out,
    std::string const& sep = " ", std::string const& inRangeSep = "--"
    ) const;
  
  
    protected:
  
  /// Default constructor: starts with no elements.
  IntegerRangesBase() = default;
  
  /// Constructor for the derived classes.
  IntegerRangesBase(std::vector<Range_t> ranges);
  
  
  /// Fills the ranges.
  template <bool CheckGrowing, typename BIter, typename EIter>
  static std::vector<Range_t> compactRange(BIter b, EIter e);
  
  
  /// Returns `value` incremented by 1.
  static constexpr Data_t plusOne(Data_t value) noexcept;
  
  /// Returns `value` decremented by 1.
  static constexpr Data_t minusOne(Data_t value) noexcept;
  
    private:
  
  std::vector<Range_t> fRanges; ///< List of current ranges.
  
  
}; // class icarus::details::IntegerRangesBase<>



// -----------------------------------------------------------------------------
/**
 * @brief A sequence of contiguous ranges of integral numbers.
 * @tparam T type of the integral numbers
 * @tparam CheckGrowing if `true`, checks will be performed on construction
 * 
 * This class parses a sequence in input grouping the consecutive elements.
 * The current interface is very simple, allowing only for query of groups
 * ("ranges") and printing to a stream.
 * The input is required and assumed to be a monotonously growing sequence,
 * with the exception that duplicate consecutive entries are allowed
 * (and ignored).
 * 
 * Each range is stored as a semi-open interval: [ _lower_, _upper_ [.
 * 
 * If `CheckGrowing` is `true`, on input an exception will be thrown if the
 * input is not strictly sorted (but duplicate elements are still allowed).
 * 
 * Example of usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * std::array data { 1, 2, 4, 5, 6, 8, 10 };
 * 
 * icarus::IntegerRanges ranges { data };
 * std::cout << "Ranges: " << ranges << std::endl;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * will print something like `Ranges: 1 2 4--6 8 10`.
 * 
 */
template <typename T /* = int */, bool CheckGrowing /* = false */>
class icarus::IntegerRanges: public icarus::details::IntegerRangesBase<T> {
  
  using Base_t = icarus::details::IntegerRangesBase<T>;
  
    public:
  static constexpr bool IsChecked = CheckGrowing;
  
  using Data_t = typename Base_t::Data_t;
  
  /// Default constructor: an empty set of ranges.
  IntegerRanges() = default;
  
  /// Constructor: range from the values pointed between `b` and `e` iterators.
  template <typename BIter, typename EIter>
  IntegerRanges(BIter b, EIter e);
  
  IntegerRanges(std::initializer_list<Data_t> data);
  
}; // class icarus::IntegerRanges<>


namespace icarus {
  // deduction guide: construct a range with the same type as the iterator value
  template <typename BIter, typename EIter>
  IntegerRanges(BIter b, EIter e)
    -> IntegerRanges<typename std::iterator_traits<BIter>::value_type>;
}


// -----------------------------------------------------------------------------
/// Returns a `IntegerRanges` object from the elements in `coll`.
template <bool CheckGrowing, typename Coll>
auto icarus::makeIntegerRanges(Coll const& coll)
  -> IntegerRanges<typename Coll::value_type, CheckGrowing>
{
  return IntegerRanges<typename Coll::value_type, CheckGrowing>
    { begin(coll), end(coll) };
} // icarus::makeIntegerRanges(Coll const& coll)



// -----------------------------------------------------------------------------
// --- template implementation
// -----------------------------------------------------------------------------
// --- icarus::details::IntegerRangesBase<>::Range_t
// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr icarus::details::IntegerRangesBase<T>::Range_t::Range_t
  (Data_t lower, Data_t upper) noexcept
  : lower(lower), upper(upper)
  {}


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr bool icarus::details::IntegerRangesBase<T>::Range_t::empty
  () const noexcept
  { return lower == upper; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr std::size_t icarus::details::IntegerRangesBase<T>::Range_t::size
  () const noexcept
  { return upper - lower; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr bool icarus::details::IntegerRangesBase<T>::Range_t::isOne
  () const noexcept
  { return plusOne(lower) == upper; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr bool icarus::details::IntegerRangesBase<T>::Range_t::isTwo
  () const noexcept
  { return plusOne(lower) == minusOne(upper); }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
void icarus::details::IntegerRangesBase<T>::Range_t::dump
  (std::ostream& out, std::string const& sep, std::string const& simpleSep)
  const
{
  
  if (empty()) {
    // let's say we don't print nothing at all
    return;
  }
  
  out << lower;
  if (isOne()) return;
  
  out << (isTwo()? simpleSep: sep) << icarus::details::IntegerRangesBase<T>::minusOne(upper);
  return;
  
} // icarus::details::IntegerRangesBase<>::Range_t::dump()


// -----------------------------------------------------------------------------
template <typename T /* = int */>
void icarus::details::IntegerRangesBase<T>::Range_t::dump
  (std::ostream& out, std::string const& sep /* = "--" */) const
  { dump(out, sep, sep); }


// -----------------------------------------------------------------------------
// --- icarus::details::IntegerRangesBase<>
// -----------------------------------------------------------------------------
template <typename T /* = int */>
icarus::details::IntegerRangesBase<T>::IntegerRangesBase
  (std::vector<Range_t> ranges): fRanges(std::move(ranges))
  {}


// -----------------------------------------------------------------------------
template <typename T /* = int */>
void icarus::details::IntegerRangesBase<T>::clear() noexcept
  { return fRanges.clear(); }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
bool icarus::details::IntegerRangesBase<T>::empty() const noexcept
  { return fRanges.empty(); }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
std::size_t icarus::details::IntegerRangesBase<T>::size() const noexcept {
  
  return std::accumulate(fRanges.begin(), fRanges.end(), 0U, 
    [](std::size_t s, Range_t const& r){ return s + r.size(); });
  
} // icarus::details::IntegerRangesBase<>::size()


// -----------------------------------------------------------------------------
template <typename T /* = int */>
std::size_t icarus::details::IntegerRangesBase<T>::nRanges() const noexcept
  { return fRanges.size(); }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
decltype(auto) icarus::details::IntegerRangesBase<T>::ranges() const noexcept
  { return fRanges; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
template <bool CheckGrowing, typename BIter, typename EIter>
auto icarus::details::IntegerRangesBase<T>::compactRange(BIter b, EIter e)
  -> std::vector<Range_t>
{
  if (b == e) return {};
  
  std::vector<Range_t> ranges;
  
  auto it = b;
  auto iPrev = it; // not sure if BIter default-constructible, so copy instead
  auto iFirst = b;
  
  while (it != e) {
    
    iPrev = it++;
    
    if (it != e) { // check current and previous elements
      if (*iPrev == *it) continue; // duplicate entry: quietly skip
      if constexpr (CheckGrowing) {
        if (*it < *iPrev) {
          using std::to_string;
          throw std::runtime_error{ "icarus::IntegerRanges"
            " initialized with non-monotonically growing sequence ("
            + to_string(*iPrev) + " then " + to_string(*it)
            + ")"
            };
        }
      } // if checking growth
    } // if not at the end
    
    auto const nextExpected = plusOne(*iPrev);
    
    if ((it != e) && (*it == nextExpected)) continue; // contiguous to previous
    
    ranges.emplace_back(*iFirst, nextExpected);
    
    iFirst = it;
    
  } // while
  
  return ranges;
} // icarus::details::IntegerRangesBase<>::compactRange()


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr auto icarus::details::IntegerRangesBase<T>::plusOne
  (Data_t value) noexcept -> Data_t
  { return ++value; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
constexpr auto icarus::details::IntegerRangesBase<T>::minusOne
  (Data_t value) noexcept -> Data_t
  { return --value; }


// -----------------------------------------------------------------------------
template <typename T /* = int */>
void icarus::details::IntegerRangesBase<T>::dump(
  std::ostream& out,
  std::string const& sep /* = " " */,
  std::string const& inRangeSep /* = "--" */
) const {
  
  if (empty()) return;
  
  auto iRange = fRanges.begin();
  auto const rend = fRanges.end();
  iRange->dump(out, inRangeSep, sep);
  while (++iRange != rend) iRange->dump(out << sep, inRangeSep, sep);
  
} // icarus::details::IntegerRangesBase<>::dump()


// -----------------------------------------------------------------------------
// --- icarus::IntegerRanges<>
// -----------------------------------------------------------------------------
template <typename T /* = int */, bool CheckGrowing /* = true */>
template <typename BIter, typename EIter>
icarus::IntegerRanges<T, CheckGrowing>::IntegerRanges(BIter b, EIter e)
  : Base_t{ Base_t::template compactRange<CheckGrowing>(b, e) }
  {}


// -----------------------------------------------------------------------------
template <typename T /* = int */, bool CheckGrowing /* = true */>
icarus::IntegerRanges<T, CheckGrowing>::IntegerRanges
  (std::initializer_list<Data_t> data)
  : IntegerRanges(data.begin(), data.end()) {}


// -----------------------------------------------------------------------------
template <typename T, bool CheckGrowing>
std::ostream& icarus::operator<<
  (std::ostream& out, typename IntegerRanges<T, CheckGrowing>::Range_t const& r)
  { r.dump(out); return out; }


// -----------------------------------------------------------------------------
template <typename T, bool CheckGrowing>
std::ostream& icarus::operator<<
  (std::ostream& out, IntegerRanges<T, CheckGrowing> const& r)
  { r.dump(out); return out; }


// -----------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_INTEGERRANGES_H
