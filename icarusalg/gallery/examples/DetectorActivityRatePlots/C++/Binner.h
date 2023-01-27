/**
 * @file    Binner.h
 * @brief   Helper object to describing a binned axis.
 * @author  Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date    October 13, 2020
 * 
 * This library is header only.
 */

#ifndef ICARUSALG_GALLERY_DETECTORACTIVITYRATEPLOTS_BINNER_H
#define ICARUSALG_GALLERY_DETECTORACTIVITYRATEPLOTS_BINNER_H


// C/C++ standard libraries
#include <ostream>
#include <algorithm> // std::clamp()
#include <utility> // std::declval()
#include <cmath> // std::ceil(), std::floor()
#include <cassert>


// -----------------------------------------------------------------------------
namespace util {
  template <typename T> class Binner; 
  template <typename T>
  std::ostream& operator<< (std::ostream&, Binner<T> const&);
} // namespace util

/**
 * @brief Helper class binning values in a range.
 * @tparam T type of data on the axis
 *
 * This object provides binning and indexing of a range of values from
 * `lower()` to `upper()`.
 * The range is divided in `nBins()` bins all of the same size `step()`.
 * 
 * The type of the values is `T`; the type of `step()` may be `T` or something
 * else, automatically detected.
 * 
 * Example of usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * util::Binner<float> const bins(-1.0f, +1.0f, 0.1f);
 * 
 * std::cout << "Binning: " << bins << std::endl; // print binning on screen
 * 
 * // print the bin index for some known values
 * std::cout << "\nBin indices (no range constraint):";
 * for (float const value: { -1.5f, -1.0f, -0.5f, 0.0f, +0.5f, +1.0f, +1.5f })
 *   std::cout << " - bin index for " << value << ": " << bins(value) << std::endl;
 * std::cout << std::flush;
 * 
 * // print the bin index for some known values
 * std::cout << "\nBin relative indices (no range constraint):";
 * for (float const value: { -1.5f, -1.0f, -0.5f, 0.0f, +0.5f, +1.0f, +1.5f })
 *   std::cout << "\n - " << value << " => [" << bins.relative(value) << "]";
 * std::cout << std::flush;
 * 
 * // print the capped bin index for some known values
 * std::cout << "\nBin indices (capped):";
 * for (float const value: { -1.5f, -1.0f, -0.5f, 0.0f, +0.5f, +1.0f, +1.5f }) {
 *   std::cout << "\n - " << value << " => [" << bins.cappedBin(value) << "]";
 * std::cout << std::flush;
 * 
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * Note that the upper bound of each bin does _not_ belong to that bin, but
 * rather to the next one (and, differently from ROOT, this also holds for
 * the last bin, i.e. the `upper()` value).
 * 
 */
template <typename T>
class util::Binner {
    public:
  using Data_t = T; ///< Type of values on the binning axis.
  
  /// Type of difference between binning axis values.
  using Step_t = decltype(std::declval<Data_t>() - std::declval<Data_t>());
  
  
  /**
   * @brief Constructor: covers the range from `lower` to `upper` or above.
   * @param lower lower bound of the binning range (used exact)
   * @param upper upper bound of the binning range (see description)
   * @param step bin width (used exact)
   * 
   * The binning range is defined to include an integral number of bins all
   * with width `step`, starting exactly at `lower` value.
   * The bins are enough that the last one includes or touches the `upper`
   * value. In other words the number of bins is set by the formula
   * @f$ \lceil (upper - lower) / step \rceil @f$, and the actual `upper()`
   * value is computed accordingly, to accommodate that many bins.
   */
  Binner(Data_t lower, Data_t upper, Step_t step);
  
  
  // -- BEGIN -- Access --------------------------------------------------------
  /// @name Access
  /// @{
  /// Returns the lower limit of the range.
  Data_t lower() const { return fLower; }
  
  /// Returns the upper limit of the range.
  Data_t upper() const { return fUpper; }
  
  /// Returns the step size.
  Step_t step() const { return fStep; }
  
  /// Returns the number of bins in the range.
  unsigned int nBins() const { return fNBins; }
  /// @}
  // -- END -- Access ----------------------------------------------------------
  
  
  // -- BEGIN -- Bin index queries ---------------------------------------------
  /**
   * @name Bin index queries
   * 
   * Type of supported queries:
   * * whether a value is within the range: `contains()`
   * * bin "index" for a value:
   *     * fractional value (`relative()`): a value in the middle of the bin
   *       will have a index with fractional part `0.5`;
   *     * integral value, with different treatment for overflows:
   *         * `bin()` (also `operator()`): no range check at all, index can be
   *           any integral value
   *         * `cappedBin()`: if the bin index is smaller, or larger, than the
   *           specified minimum and maximum bin numbers, the returned value is
   *           clamped to those minimum and maximum bin numbers;
   *         * `cappedBin()`: the bin index is between `0` and `nBins() - 1`,
   *           where bin `0` also includes all values smaller than `lower()`
   *           and bin `nBins() - 1` includes all values larger or equal to
   *           `upper()`
   *         * `cappedBinWithOverflows()`: the bin index is between `-1` and
   *           `nBins()`, where bin `-1` includes all values smaller than
   *           `lower()` and bin `nBins()` includes all values larger or equal
   *           to `upper()`
   */
  /// @{
  
  // @{
  /// Returns `value` relative to the range (lower = 0, upper = 1).
  double relative(Data_t value) const { return (value - fLower) / fStep; }
  // @}
  
  // @{
  /// Returns bin number for `value` (unbound).
  int bin(Data_t value) const
    { return static_cast<int>(std::floor(relative(value))); }
  int operator() (Data_t value) const { return bin(value); }
  // @}
  
  // @{
  /// Returns a bin number for `value` clamped between `min` and `max` included.
  int cappedBin(Data_t value, int min, int max) const
    { return std::clamp(bin(value), min, max); }
  // @}
  
  // @{
  /// Returns a valid bin index, capping if `value` is out of range.
  int cappedBin(Data_t value) const { return cappedBin(value, 0, nBins() - 1); }
  // @}
  
  // @{
  /// Returns a valid bin index or `-1` for underflow or `nBins()` for overflow.
  int cappedBinWithOverflows(Data_t value) const
    { return cappedBin(value, -1, nBins()); }
  // @}
  
  /// @}
  // -- END -- Bin index queries -----------------------------------------------
  
  
  // -- BEGIN -- Range queries -------------------------------------------------
  /// @name Range queries
  /// @{
  
  /// Returns if `value` is in the range.
  bool contains(Data_t value) const
    { return (value >= fLower) && (value < fUpper); }
  
  /// Returns the lower edge of the bin with the specified index `iBin`.
  Data_t lowerEdge(int iBin) const { return fLower + fStep * iBin; }
  
  /// Returns the upper edge of the bin with the specified index `iBin`.
  Data_t upperEdge(int iBin) const { return lowerEdge(iBin + 1); }
  
  /// Returns the center of the bin with the specified index `iBin`.
  Data_t binCenter(int iBin) const { return lowerEdge(iBin) + fStep / 2; }
  
  /// @}
  // -- END -- Range queries ---------------------------------------------------
  
  
    private:
  Data_t fLower; ///< Lower bound of the covered range.
  Step_t fStep; ///< Width of the bins.
  unsigned int fNBins; ///< Number of bins in the range.
  Data_t fUpper; ///< Upper bound of the covered range.
  
}; // util::Binner<>


// -----------------------------------------------------------------------------
// ---  template implementation
// -----------------------------------------------------------------------------
template <typename T>
util::Binner<T>::Binner(Data_t lower, Data_t upper, Step_t step)
  : fLower(lower)
  , fStep(step)
  , fNBins(static_cast<unsigned int>(std::ceil((upper - lower) / step)))
  , fUpper(lowerEdge(fNBins))
  { assert(lower <= upper); }


// -----------------------------------------------------------------------------
template <typename T>
std::ostream& util::operator<< (std::ostream& out, Binner<T> const& binner) {
  
  out << "[ " << binner.lower() << " -- " << binner.upper() << " ] ("
    << binner.nBins() << "x " << binner.step() << ")";
  return out;
  
} // operator<< (Binner<>)


// -----------------------------------------------------------------------------

#endif // ICARUSALG_GALLERY_DETECTORACTIVITYRATEPLOTS_BINNER_H
