/**
 * @file   icarusalg/Utilities/PlotSandbox.h
 * @brief  A helper to manage ROOT objects in a `art::TFileDirectory`.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 8, 2019
 * @see    `icarusalg/Utilities/PlotSandbox.tcc`
 */

#ifndef ICARUSALG_UTILITIES_PLOTSANDBOX_H
#define ICARUSALG_UTILITIES_PLOTSANDBOX_H

// framework libraries
#include "cetlib_except/exception.h"

// ROOT libraries
#include "TDirectory.h"

// C/C++ standard libraries
#include <initializer_list>
#include <string>
#include <map>
#include <utility> // std::move(), std::pair<>
#include <memory> // std::unique_ptr<>
#include <functional> // std::hash<>


//------------------------------------------------------------------------------
namespace icarus::ns::util {
  
  template <typename DirectoryBackend> class PlotSandbox;
  
  namespace details {
    template <typename Map>
    decltype(auto) map_dereferenced_values(Map&& map);
    
    template <typename Backend> class TDirectoryHelper;
  }
  
} // namespace icarus::ns::util

/**
 * @brief A helper to manage ROOT objects with consistent naming.
 * @tparam DirectoryBackend type enclosing the interaction with the output file
 *
 * A sandbox includes a ROOT directory where all the objects are written.
 * It also provides a name pattern to modify a generic object name in one
 * specific to this sandbox, e.g. from `"HEnergy"` to `"HMuonNeutrinoEnergy"`.
 * Object descriptions (usually titles) may also be processed in the same way.
 * 
 * In addition, the sandbox can point to a parent sandbox, in which case the
 * names and descriptions are first processed by this sandbox and then passed to
 * that parent sandbox for further processing.
 * 
 * The sandbox has two characterizing strings:
 * * the _name_ is a key used in ROOT object names and it is also by default
 *   the name of the output ROOT directory; for example: `"MuonNeutrino"` or
 *   `"CC"`. This string becoming part of the ROOT object name, it is better to
 *   keep it short and simply alphanumeric.
 * * the _description_ is a short wording for the content of the sandbox, used
 *   in processing ROOT object titles; for example, `"#nu_{#mu}"` or
 *   `"charged current"`.
 * 
 * @note By convention the subdirectory names are not processed.
 * 
 * 
 * This utility class is expected to work both within and without _art_.
 * When _art_ is available, `DirectoryBackend` can be set to use
 * `art::TFileDirectory`, while in a pure ROOT environment `TDirectoryFile`
 * (from ROOT) can be used instead.
 * 
 */
template <typename DirectoryBackend>
class icarus::ns::util::PlotSandbox {
    public:
  using PlotSandbox_t = PlotSandbox<DirectoryBackend>;
  
    private:
  
  /// Type of object for interfacing to ROOT TDirectory.
  using DirectoryHelper_t = details::TDirectoryHelper<DirectoryBackend>;
  
  /// The whole data in a convenient package!
  struct Data_t {
    
    std::string name; ///< The name/key representing this sandbox.
    std::string desc; ///< The description characterizing the sandbox.
    
    /// Optional parent sandbox.
    PlotSandbox_t const* parent = nullptr;
    
    /// Contained sand boxes.
    std::map<std::string, std::unique_ptr<PlotSandbox_t>> subBoxes;
    
    DirectoryHelper_t outputDir; ///< Output ROOT directory of the sandbox.
    
    Data_t() = default;
    Data_t(Data_t const&) = delete;
    Data_t(Data_t&&) = default;
    Data_t& operator= (Data_t const&) = delete;
    Data_t& operator= (Data_t&&) = default;
    
    Data_t
      (std::string&& name, std::string&& desc, DirectoryHelper_t outputDir)
      : name(std::move(name)), desc(std::move(desc))
      , outputDir(std::move(outputDir))
      {}
    
    void resetSubboxParents(PlotSandbox_t const* newParent);
    
  } fData;
  
  
  /// Helper function for `findSandbox()` implementations.
  template <typename SandboxType>
  static PlotSandbox_t* findSandbox
    (SandboxType& sandbox, std::string const& name);
  
  /// Helper function for `demandSandbox()` implementations.
  template <typename SandboxType>
  static PlotSandbox_t& demandSandbox
    (SandboxType& sandbox, std::string const& name);
  
  
    public:
  
  struct NoNameTitle_t {}; ///< Special type for marking `make()` parameters.
  
  /// Special value for marking `make()` parameters.
  static constexpr NoNameTitle_t NoNameTitle {};
  
  /**
   * @brief Constructor: specifies all sandbox characteristics.
   * @param parentDir ROOT directory under which the sandbox is created
   * @param name the name of the sandbox
   * @param desc description of the sandbox
   * 
   * If the `name` is empty, as a special case, the sandbox is unnamed and it is
   * created directly in `parentDir`. In that case, the title of the output
   * directory (`parentDir`) is also not changed.
   * If `name` or `desc` are empty, the pertaining processing is not performed:
   * if `name` is empty, ROOT object names will be unchanged, and if `desc` is
   * empty their descriptions/titles will be.
   * 
   * To create a sandbox with a parent, call `addSubSandbox()` of that parent.
   */
  PlotSandbox(DirectoryBackend parentDir, std::string name, std::string desc);
  
  PlotSandbox(PlotSandbox_t const&) = delete;
  PlotSandbox(PlotSandbox_t&& from);
  
  // no assignment supported in `art::TFileDirectory`
  PlotSandbox& operator=(PlotSandbox_t const&) = delete;
  PlotSandbox& operator=(PlotSandbox_t&& from) = delete;
  
  
  /// Virtual destructor. Default, but C++ wants it.
  virtual ~PlotSandbox() = default;
  
  /// Returns whether we have a non-empty name.
  bool hasName() const { return !fData.name.empty(); }
  
  /// Returns the sandbox name.
  std::string const& name() const { return fData.name; }
  
  /// Returns whether we have a non-empty description.
  bool hasDescription() const { return !fData.desc.empty(); }
  
  /// Returns the sandbox description.
  std::string const& description() const { return fData.desc; }
  
  /// Returns the sandbox description for use at the beginning of a sentence.
  /// @todo Not implemented yet.
  virtual std::string const& Description() const { return description(); }
  
  /// Returns a string ID for this sandbox.
  std::string ID() const;
  
  /// Processes the specified string as it were a name.
  virtual std::string processName(std::string const& name) const;
  
  /// Processes the specified string as it were a description or title.
  virtual std::string processTitle(std::string const& title) const;
  
  
  // --- BEGIN -- ROOT object management ---------------------------------------
  /// @name ROOT object management
  /// @{
  
  /// Returns if the sandbox is empty (neither it nor subboxes hold objects).
  bool empty() const;
  
  /**
   * @brief Fetches the object with the specified name from the sandbox.
   * @tparam Obj (default: `TObject`) type of the object to fetch
   * @param name unprocessed name and path of the object to fetch
   * @return a constant pointer to the requested object , or `nullptr` if not
   *         available or wrong type
   * @see `use()`
   * 
   * The `name` specification may contain a ROOT path. The directory component
   * of the path, defined by everything preceding a `/` character, is _not_
   * processed.
   * 
   * The fetched object is converted to the desired type via `dynamic_cast`.
   * If conversion fails, a null pointer is returned.
   */
  template <typename Obj = TObject>
  Obj const* get(std::string const& name) const;
  
  /**
   * @brief Fetches an object with the specified name to be modified.
   * @tparam Obj (default: `TObject`) type of the object to fetch
   * @param name unprocessed name and path of the object to fetch
   * @return a pointer to the requested object , or `nullptr` if not
   *         available or wrong type
   * @see `get()`
   * 
   * This method is fully equivalent to `get()`, with the difference that the
   * returned object may be modified.
   */
  template <typename Obj = TObject>
  Obj* use(std::string const& name) const;
  
  /**
   * @brief Fetches an object with the specified name to be modified.
   * @tparam Obj (default: `TObject`) type of the object to fetch
   * @param name unprocessed name and path of the object to fetch
   * @return the requested object
   * @throw cet::exception (category: `"PlotSandbox"`) if no object with `name`
   *        exists in the box
   * @see `get()`, `use()`
   * 
   * This method is equivalent to `use()`, with the difference that the
   * returned object must exist.
   */
  template <typename Obj = TObject>
  Obj& demand(std::string const& name) const;
  
  /**
   * @brief Fetches the base directory of the sandbox.
   * @return a pointer to the requested directory, or `nullptr` if wrong type
   * 
   * The directory is converted to the desired type via `dynamic_cast`.
   * If conversion fails, a null pointer is returned.
   */
  template <typename DirObj = TDirectory>
  DirObj* getDirectory() const;

  /**
   * @brief Fetches the directory with the specified name from the sandbox.
   * @tparam DirObj (default: `TDirectory`) type of ROOT directory object to get
   * @param path path of the directory to fetch within this sandbox
   * @return a constant pointer to the requested directory, or `nullptr` if not
   *         available or wrong type
   * 
   * The fetched object is converted to the desired type via `dynamic_cast`.
   * If conversion fails, a null pointer is returned.
   */
  template <typename DirObj = TDirectory>
  DirObj* getDirectory(std::string const& path) const;
  
  /**
   * @brief Creates a new ROOT object with the specified name and title.
   * @tparam Obj type of ROOT object to be created
   * @tparam Args types of the arguments to be forwarded to the constructor
   * @param name unprocessed name of the new object
   * @param title unprocessed title of the new object
   * @param args additional arguments forwarded to the constructor
   * @return a pointer to the newly created object
   * 
   * The name and title are processed with `processName()` and `processTitle()`
   * method respectively, before the object is created.
   * 
   * @note By default, the constructor of `Obj` is given arguments the name and
   *       the title (both as C strings) and then all `args`, forwarded.
   *       If the first of the `Args` is `NoNameTitle`, only `args` are passed
   *       instead.
   * 
   */
  template <typename Obj, typename... Args>
  Obj* make(std::string const& name, std::string const& title, Args&&... args);
  
  
  /**
   * @brief Acquires an object.
   * @tparam Obj type of the object to acquire
   * @param obj pointer to the object to be acquired
   * @param name new registered base name (`obj` name by default)
   * @param title new registered base title (`obj` title by default)
   * @return a pointer to the owned object
   * 
   * A new object is created (like with `make()`), constructed from moving the
   * object in `obj`.
   * The `name` and `title` may be changed, and then they are processed to
   * comply with the sandbox standards.
   * 
   * The object `Obj` must, in addition to the standard requirements of
   * `make<Obj>()`, also properly support the move constructor.
   */
  template <typename Obj, typename... Args>
  Obj* acquire(
    std::unique_ptr<Obj>&& obj,
    std::string const& name = "", std::string const& title = ""
    );
  
  
  /// @}
  // --- END -- ROOT object management -----------------------------------------
  
  
  // --- BEGIN -- Contained sandboxes ------------------------------------------
  /// @name Contained sandboxes
  /// @{
  
  /**
   * @brief Creates a new sandbox contained in this one.
   * @tparam SandboxType (default: `PlotSandbox`) type of sandbox to be created
   * @tparam Args types of the additional arguments for the sandbox constructor
   * @param name name of the new contained sand box
   * @param desc description of the new contained sand box
   * @param args additional arguments for the sandbox constructor
   * @return a reference to the created sandbox
   * @throw cet::exception (category: `"PlotSandbox"`) if a sandbox with this
   *        name already exists.
   * 
   * The arguments of this method are equivalent to the ones of the constructor.
   * 
   * The new sand box parent is set to point to this sand box.
   * 
   * Note that subboxes in the path are not created: the parent of the sandbox
   * pointed by `name` must exist.
   */
  template <typename SandboxType = PlotSandbox_t, typename... Args>
  SandboxType& addSubSandbox(
    std::string const& name, std::string const& desc,
    Args&&... args
    );
  
  /// Returns the number of contained sand boxes.
  std::size_t nSubSandboxes() const { return fData.subBoxes.size(); }
  
  // @{
  /**
   * @brief Returns the first contained sandbox with the specified name.
   * @param name unprocessed name of the box to be retrieved
   * @return the requested contained sandbox, or `nullptr` if not found
   * @see `demandSandbox()`
   * 
   * Full sandbox paths, separated by a '/' character, are supported.
   * The function returns `nullptr` if any sandbox in the path is not found.
   */
  PlotSandbox_t const* findSandbox(std::string const& name) const;
  PlotSandbox_t* findSandbox(std::string const& name);
  // @}
  
  // @{
  /**
   * @brief Returns the first contained sandbox with the specified name.
   * @param name unprocessed name of the box to be retrieved
   * @return the requested contained sandbox
   * @throw cet::exception (category: `"PlotSandbox"`) if no a sandbox with this
   *        `name` exists
   * @see `findSandbox()`
   * 
   * Full sandbox paths, separated by a '/' character, are supported.
   * The function returns `nullptr` if any sandbox in the path is not found.
   */
  PlotSandbox_t const& demandSandbox(std::string const& name) const;
  PlotSandbox_t& demandSandbox(std::string const& name);
  // @}
  
  // @{
  /// Returns an object proper to iterate through all contained sand boxes.
  decltype(auto) subSandboxes() const
    { return details::map_dereferenced_values(fData.subBoxes); }
  decltype(auto) subSandboxes()
    { return details::map_dereferenced_values(fData.subBoxes); }
  // @}
  
  /**
   * @brief Deletes the subbox with the specified `name` and its directory.
   * @return whether there was a subbox with that `name`
   * 
   * Full sandbox paths, separated by a '/' character, are supported.
   * The function returns `false` if any sandbox in the path is not found.
   */
  bool deleteSubSandbox(std::string const& name);
  
  /// @}
  // --- END -- Contained sandboxes --------------------------------------------
  
  
  /// Dumps the hierarchy of sandboxes into the specified stream.
  template <typename Stream>
  void dump(Stream&& out, std::string indent, std::string firstIndent) const;
  
  /// Dumps the hierarchy of sandboxes into the specified stream.
  template <typename Stream>
  void dump(Stream&& out, std::string indent = "") const
    { dump(std::forward<Stream>(out), indent, indent); }
  
    protected:
  
  /**
   * @brief Constructor: specifies all sandbox characteristics.
   * @param parent the sandbox used as parent
   * @param name the name of the sandbox
   * @param desc description of the sandbox
   * 
   * Compared to the public constructor, this one picks the parent directory
   * to be the output directory of the `parent` sandbox, and it registers
   * that `parent` box as parent.
   * Note that this constructor does *not* update the list of subboxes of the
   * parent.
   */
  PlotSandbox(PlotSandbox_t const& parent, std::string name, std::string desc);
  
  
  /// Sets the parent of this box.
  virtual void setParent(PlotSandbox_t const* parent) { fData.parent = parent; }
  
  
  /// Applies title processing only at the title part of the string.
  std::string processPlotTitle(std::string const& title) const;


  /// Returns a processed version of the name of this sandbox.
  /// 
  /// This may be used when integrating the sandbox name into the processed
  /// object names.
  virtual std::string processedSandboxName() const;

  /// Returns a processed version of the description of this sandbox.
  /// 
  /// This may be used when integrating the sandbox description into the
  /// processed object descriptions.
  virtual std::string processedSandboxDesc() const;

  /// Dumps the content of this box (nosubboxes) into `out` stream.
  template <typename Stream>
  void dumpContent
    (Stream&& out, std::string indent, std::string firstIndent) const;
  
  
  /// Returns a pair with the first directory and the rest of the path.
  /// The directory part may be empty.
  static std::pair<std::string, std::string> peelDir
    (std::string const& path, char sep = '/');
  
  /// Returns a pair with the directory and the name part of `path`.
  /// The directory part may be empty.
  static std::pair<std::string, std::string> splitPath
    (std::string const& path, char sep = '/');
  
  /// Merges the pieces of path that are not empty into a path.
  /// One separator at the end of each piece is ignored.
  static std::string joinPath
    (std::initializer_list<std::string> pathElements, char sep = '/');
  
  
    private:
  
  template <typename Obj, typename... Args>
  Obj* makeImpl(
    DirectoryHelper_t destDir,
    std::string const& processedName, std::string const& processedTitle,
    Args&&... args
    ) const;
  template <typename Obj, typename... Args>
  Obj* makeImpl(
    DirectoryHelper_t destDir,
    std::string const& processedName, std::string const& processedTitle,
    NoNameTitle_t, Args&&... args
    ) const;
  
}; // icarus::ns::util::PlotSandbox


//------------------------------------------------------------------------------
//---  Standard library support
//------------------------------------------------------------------------------
namespace std {
  
  template <typename DirectoryBackend>
  struct hash<icarus::ns::util::PlotSandbox<DirectoryBackend>> {
    auto operator()
      (icarus::ns::util::PlotSandbox<DirectoryBackend> const& key) const
      { return std::hash<std::string>()(key.ID()); }
  }; // hash<PlotSandbox>
  
} // namespace std


//------------------------------------------------------------------------------
//--- template implementation
//------------------------------------------------------------------------------
#include "PlotSandbox.tcc"

//------------------------------------------------------------------------------


#endif // ICARUSALG_UTILITIES_PLOTSANDBOX_H
