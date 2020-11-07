/**
 * @file   icarusalg/Utilities/SampledFunction.h
 * @brief  Class for a function with precomputed values.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   February 14, 2020
 *
 * This is a header-only library.
 */

#ifndef ICARUSALG_UTILITIES_SAMPLEDFUNCTION_H
#define ICARUSALG_UTILITIES_SAMPLEDFUNCTION_H

// LArSoft libraries
#include "larcorealg/CoreUtils/enumerate.h"
#include "larcorealg/CoreUtils/counter.h"

// C++ core guideline library
#include "gsl/span"
#include "gsl/gsl_util" // gsl::index

// C++ standard library
#include <vector>
#include <functional> // std::function<>
#include <limits> // std::numeric_limits<>
#include <cmath> // std::isnormal()
#include <cassert>


// ---------------------------------------------------------------------------
namespace util {
  template <typename XType, typename YType> class SampledFunction;
} // namespace util

/**
 * @brief Precomputed discrete sampling of a given function.
 * @tparam XType (default: `double`) type of value accepted by the function
 * @tparam YType (default: as `XType`) type of value returned by the function
 *
 * This object contains the sampling of a specified function at regular values
 * of its variable.
 *
 * If the `size()` of the sampling is requested to be _N_, there will be a
 * sampling of _N_ values covering the specified range in steps of the same
 * length, last value excluded.
 * The sampling happens at the beginning of each step.
 *
 * In addition, subsampling can be requested. If _M_ subsamples are requested,
 * the first step is split in _M_ points and from each one a sampling of _N_
 * steps is started, causing overall _M N_ samples to be computed.
 *
 *
 * Requirements
 * -------------
 *
 * The function must be unary.
 *
 *
 * Technical note
 * ---------------
 *
 * The _M_ subsamples are stored each one contiguously.
 * Therefore a function with _M_ subsamples of size _N_ is different, at least
 * in storage, from a function with a single sampling (no subsamples) of size
 * _M N_.
 *
 * @note Due to the implementation of `gsl::span`, its iterators are valid only
 *       while the span object is valid as well.
 *       This means that a construct like:
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *       for (auto it = sf.subsample(0).begin(); it != sf.subsample(0).end(); ++it)
 *         // ...
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *       will mysteriously fail at run time because `it` refers to a temporary
 *       span that quickly falls out of scope (and the end iterator refers to
 *       a different span object, too). The correct pattern is to store the
 *       subsample result before iterating through it:
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *       auto const& subsample = sf.subsample(0);
 *       for (auto it = subsample.begin(); it != subsample.end(); ++it)
 *         // ...
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *       or, if appliable, let the range-for loop di that for us:
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *       for (auto value: sf.subsample(0))
 *         // ...
 *       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *       This will not be the case any more with `std::span`, which apparently
 *       is going to satisfy the `borrowed_range` requirement.
 *
 */
template <typename XType = double, typename YType = XType>
class util::SampledFunction {
    public:

  using X_t = XType; ///< Type of value accepted by the function.
  using Y_t = YType; ///< Type of value returned by the function.
  using Function_t = std::function<Y_t(X_t)>; ///< Type of sampled function.

  /// Invalid index of sample, returned in case of error.
  static constexpr auto npos = std::numeric_limits<gsl::index>::max();

  /// Span of subsample data. Can be forward iterated.
  using SubsampleData_t = gsl::span<Y_t const>;

  SampledFunction() = default; // FIXME remove this
  
  /**
   * @brief Constructor: samples `function` in the specified range.
   * @tparam Func type of a functor (see requirements below)
   * @param function the function to be sampled
   * @param lower the lower limit of the range to be sampled
   * @param upper the upper limit of the range to be sampled
   * @param nSamples the number (_N_) of samples to be computed
   * @param subsamples (default: `1`) the number (_M_) of subsamples to be
   *                   computed
   *
   * The sampling of `function` is performed on `nSamples` points from `lower`
   * to `upper` (excluded).
   *
   * The `function` parameter type `Func` need to be a unary functor, i.e. it
   * must support a call of type `function(X_t) const` returning some value
   * convertible to `Y_t`. Plain C functions and closures also work.
   *
   * The `function` is not copied nor retained in any form, so it can be from
   * a temporary object.
   */
  template <typename Func>
  SampledFunction(Func const& function,
    X_t lower, X_t upper,
    gsl::index nSamples,
    gsl::index subsamples = 1
    );


  // @{
  /**
   * @brief Constructor: samples `function` in the specified range.
   * @tparam UntilFunc type of functor indicating when to stop sampling
   * @param function the function to be sampled
   * @param lower the lower limit of the range to be sampled
   * @param step the sampling interval
   * @param until functor to indicate when to stop sampling
   * @param subsamples (default: `1`) the number (_M_) of subsamples to be
   *                   computed
   * @param min_upper (default: none) minimum value covered by the sampling
   *
   * The sampling of `function` is performed from `lower`, advancing by `step`
   * at each following sample, until the `until` functor returns `true`.
   * If `min_upper` is specified, regardless of the result of `until`, samples
   * below `min_upper` are always covered.
   *
   * The functor `until` should be callable as in `bool until(X_t x, Y_t y)`,
   * and should return `false` if the sample of value `y`, corresponding to the
   * evaluation point `x`, needs to be sampled, and `true` if instead that
   * sample needs to be discarded, and the sampling stopped. For example,
   * to apply a threshold so that sampling stops when the function is 0.1,
   * `until` can be defined as `[](X_t, Y_t s){ return s >= Y_t{0.1}; }` (`x`
   * is ignored).
   *
   * Subsampling is performed based on the `subsamples` argument.
   */
  template <typename Func, typename UntilFunc>
  SampledFunction(Func const& function,
    X_t lower, X_t step, UntilFunc&& until,
    gsl::index subsamples,
    X_t min_upper
    );
  template <typename Func, typename UntilFunc>
  SampledFunction(Func const& function,
    X_t lower, X_t step, UntilFunc&& until,
    gsl::index subsamples = 1
    );
  // @}



  // --- BEGIN --- Query -------------------------------------------------------
  /// @name Query
  /// @{

  // @{
  /// Returns the number of samples (in each subsample).
  gsl::index size() const { return fNSamples; }
  // @}

  // @{
  /// Returns the number of subsamples.
  gsl::index nSubsamples() const { return fNSubsamples; }
  // @}

  // @{
  /// Returns the lower limit of the covered range.
  X_t lower() const { return fLower; }
  // @}

  // @{
  /// Returns the upper limit of the covered range.
  X_t upper() const { return fUpper; }
  // @}

  // @{
  /// Returns the extension of the covered range.
  X_t rangeSize() const { return upper() - lower(); }
  // @}

  // @{
  /// Returns the extension of a step.
  X_t stepSize() const { return fStep; }
  // @}

  // @{
  /// Returns the base offset of the subsamples.
  X_t substepSize() const { return stepSize() / nSubsamples(); }
  // @}

  /// @}
  // --- END --- Query ---------------------------------------------------------


  // --- BEGIN --- Access ------------------------------------------------------
  /// @name Access to the sampled data
  /// @{

  // @{
  /// Returns the `iSample` value of the subsample with the specified index `n`.
  Y_t value(gsl::index iSample, gsl::index n = 0U) const
    { return subsampleData(n)[iSample]; }
  // @}


  // @{
  /// Returns the data of the subsample with the specified index `n`.
  SubsampleData_t subsample(gsl::index const n) const
    { return { subsampleData(n), static_cast<gsl::index>(fNSamples) }; }
  // @}

  // @{
  /**
   * @brief Returns the index of the step including `x`.
   * @param x the argument to the function
   * @param iSubsample the index of the subsample
   * @return the index of step including `x`, or `npos` if none does
   *
   * This function returns the index of the sample whose step includes `x`.
   * A step includes its lower limit but not its upper limit, which usually
   * belongs to the next step (or it does not belong to any valid step).
   * If there is no step including `x`, the index of the would-be step is
   * returned (it can be checked e.g. with `isValidStepIndex()`).
   */
  gsl::index stepIndex(X_t const x, gsl::index const iSubsample) const;
  // @}

  // @{
  /// Returns whether the specified step index is valid.
  bool isValidStepIndex(gsl::index const index) const
    { return (index >= 0) && (index < size()); }
  // @}

  // @{
  /**
   * @brief Returns the subsample closest to the value `x`.
   * @param x value to be checked
   * @return the index of the subsample found
   *
   * The subsample with the bin including `x` whose lower bound is the closest
   * to `x` itself is returned.
   *
   * For example, assuming bins aligned with 0 and a sampling with steps of
   * size 1 and 5 subsamples, there will be 5 bins contaning the value `x` 3.65:
   * [ 3.0, 4.0 ], [ 3.2, 4.2 ], [ 3.4, 4.4 ], [ 3.6, 4.6 ] and [ 2.8, 3.8 ],
   * one for each subsample: `closestSubsampleIndex(3.65)` will return the
   * sample with the bin [ 3.6, 4.6 ] (that is the fourth one, i.e. subsample
   * number 3), because its lower bound 3.6 is the closest to 3.65.
   *
   * The value `x` does not need to be in the sampling range. In the example
   * above, the range could have been between 0 and 2, and the result would be
   * the same.
   */
  gsl::index closestSubsampleIndex(X_t x) const;
  // @}

  /// @}
  // --- END --- Access --------------------------------------------------------

  /// Dumps the full content of the sampling into `out` stream.
  template <typename Stream>
  void dump
    (Stream&& out, std::string const& indent, std::string const& firstIndent)
    const;

  /// Dumps the full content of the sampling into `out` stream.
  template <typename Stream>
  void dump(Stream&& out, std::string const& indent = "") const
    { dump(std::forward<Stream>(out), indent, indent); }


    private:

  /// Record used during initialization.
  struct Range_t {
    X_t lower, upper;
    X_t step;
    gsl::index nSamples;
  }; // Range_t

  X_t fLower; ///< Lower limit of sampled range.
  X_t fUpper; ///< Upper limit of sampled range.
  gsl::index fNSamples; ///< Number of samples in the range.
  gsl::index fNSubsamples; ///< Number of subsamples.

  X_t fStep; ///< Step size.

  /// All samples, the entire first subsample first.
  std::vector<Y_t> fAllSamples;

  /// Constructor implementation.
  SampledFunction
    (Function_t const& function, Range_t const& range, gsl::index subsamples);

  /// Returns the starting point of the subsample `n`.
  X_t subsampleOffset(gsl::index n) const
    { return lower() + substepSize() * n; }


  // @{
  /// Start of the block of values for subsample `n` (unchecked).
  Y_t const* subsampleData(gsl::index n) const
    { return fAllSamples.data() + fNSamples * n; }
  Y_t* subsampleData(gsl::index n) { return fAllSamples.data() + fNSamples * n; }
  // @}

  /// Computes the total size of the data.
  std::size_t computeTotalSize() const { return nSubsamples() * size(); }


  /// Returns a range including at least from `lower` to `min_upper`,
  /// extended enough that `until(upper, f(upper))` is `true`, and with an
  /// integral number of steps.
  template <typename UntilFunc>
  static Range_t extendRange(
    Function_t const& function, X_t lower, X_t min_upper, X_t step,
    UntilFunc&& until
    );

  /// Samples the `function` and fills the internal caches.
  void fillSamples(Function_t const& function);


  /// Returns `value` made non-negative by adding multiples of `range`.
  template <typename T>
  static T wrapUp(T value, T range);

}; // class util::SampledFunction<>


// template deduction guide:
namespace util {
  
  // when YType is not deducible (Clang 7.0.0 can't deduce it, GCC 8.2 can)
  template <typename XType, typename Func, typename UntilFunc>
  SampledFunction(Func const&, XType, XType, UntilFunc&&, gsl::index, XType)
    -> SampledFunction<XType, XType>;
  
  template <typename XType, typename Func>
  SampledFunction(Func const&, XType, XType, gsl::index, gsl::index = 1)
    -> SampledFunction<XType, XType>;
  
  template <typename XType, typename Func, typename UntilFunc>
  SampledFunction(Func const&, XType, XType, UntilFunc&&, gsl::index = 1)
    -> SampledFunction<XType, XType>;
  
} // namespace util


// =============================================================================
// ===  template implementation
// =============================================================================
template <typename XType, typename YType>
template <typename Func>
util::SampledFunction<XType, YType>::SampledFunction(
  Func const& function,
  X_t lower, X_t upper,
  gsl::index nSamples,
  gsl::index subsamples
  )
  : SampledFunction(
      Function_t(function),
      Range_t{ lower, upper, (upper - lower) / nSamples, nSamples },
      subsamples
    )
{}


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
template <typename Func, typename UntilFunc>
util::SampledFunction<XType, YType>::SampledFunction(
  Func const& function,
  X_t lower, X_t step, UntilFunc&& until,
  gsl::index subsamples,
  X_t min_upper
  )
  : SampledFunction(
      Function_t(function),
      extendRange
        (function, lower, min_upper, step, std::forward<UntilFunc>(until)),
      subsamples
    )
{}


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
gsl::index util::SampledFunction<XType, YType>::stepIndex
  (X_t const x, gsl::index const iSubsample) const
{
  auto const dx = static_cast<double>(x - subsampleOffset(iSubsample));
  return static_cast<gsl::index>(std::floor(dx / stepSize()));
} // gsl::index util::SampledFunction<XType, YType>::stepIndex()


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
gsl::index util::SampledFunction<XType, YType>::closestSubsampleIndex
  (X_t const x) const
{
  auto const dx = static_cast<double>(x - lower());
  auto const step = static_cast<double>(stepSize());
  auto const substep = static_cast<double>(substepSize());
  return static_cast<gsl::index>(wrapUp(std::fmod(dx, step), step) / substep);
} // gsl::index util::SampledFunction<XType, YType>::stepIndex()


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
template <typename Stream>
void util::SampledFunction<XType, YType>::dump
  (Stream&& out, std::string const& indent, std::string const& firstIndent) const
{
  out << firstIndent << "Function sampled from " << lower() << " to " << upper()
    << " (extent: " << rangeSize() << ")"
    << " with " << size() << " samples (" << stepSize() << " long)";
  if (nSubsamples() > 1) {
    out << " and " << nSubsamples() << " subsamples (" << substepSize()
      << " long):";
  }
  for (auto const iSub: util::counter(nSubsamples())) {
    out << "\n" << indent << "<subsample #" << iSub << ">:";
    // FIXME with C++20's `std::span` temporary won't be needed any more
    auto const& sub = subsample(iSub);
    for (auto const [ i, sample ]: util::enumerate(sub))
      out << " [" << i << "] " << sample;
  } // for
  out << "\n";
} // util::SampledFunction<>::dump()


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
util::SampledFunction<XType, YType>::SampledFunction(
  Function_t const& function,
  Range_t const& range,
  gsl::index subsamples
  )
  : fLower(range.lower)
  , fUpper(range.upper)
  , fNSamples(range.nSamples)
  , fNSubsamples(subsamples)
  , fStep(range.step)
  , fAllSamples(computeTotalSize())
{
  assert(fNSamples > 0);
  assert(subsamples > 0);
  fillSamples(function);
} // util::SampledFunction<>::SampledFunction(range)


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
template <typename UntilFunc>
auto util::SampledFunction<XType, YType>::extendRange(
  Function_t const& function, X_t lower, X_t min_upper, X_t step,
  UntilFunc&& until
  ) -> Range_t
{
  assert(min_upper >= lower);
  auto const startSamples
    = static_cast<gsl::index>(std::ceil((min_upper - lower) / step));

  auto const endStep
    = [lower, step](gsl::index iStep){ return lower + step * iStep; };

  Range_t r { lower, endStep(startSamples), step, startSamples };

  while (!until(r.upper + r.step, function(r.upper + r.step))) {
    // upper + step is not too much: extend to there
    ++r.nSamples;
    r.upper = endStep(r.nSamples);
  } // while

  return r;
} // util::SampledFunction<>::extendRange()


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
void util::SampledFunction<XType, YType>::fillSamples
  (Function_t const& function)
{

  /*
   * Plan:
   * 0. rely on the currently stored size specifications (range and samples)
   * 1. resize the data structure to the required size
   * 2. fill all the subsamples, in sequence
   *
   */

  //
  // 0. rely on the currently stored size specifications (range and samples)
  //
  std::size_t const dataSize = computeTotalSize();
  assert(dataSize > 0U);
  assert(fLower <= fUpper);
  assert(fStep > X_t{0});

  //
  // 1. resize the data structure to the required size
  //
  fAllSamples.resize(dataSize);

  //
  // 2. fill all the subsamples, in sequence
  //
  auto iValue = fAllSamples.begin();
  for (gsl::index const iSubsample: util::counter(nSubsamples())) {
    X_t const offset = subsampleOffset(iSubsample);
    for (gsl::index const iStep: util::counter(size())) {
      X_t const x = offset + iStep * stepSize();
      Y_t const y = function(x);
      *iValue++ = y;
    } // for steps
  } // for subsamples

} // util::SampledFunction<>::fillSamples()


// -----------------------------------------------------------------------------
template <typename XType, typename YType>
template <typename T>
T util::SampledFunction<XType, YType>::wrapUp(T value, T range) {
  while (value < T{ 0 }) value += range;
  return value;
} // util::SampledFunction<>::wrapUp()


// -----------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_SAMPLEDFUNCTION_H
