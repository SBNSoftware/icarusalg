/**
 * @file   icarusalg/Utilities/PlotSandbox.tcc
 * @brief  A helper to manage ROOT objects in a `art::TFileDirectory`.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   August 8, 2019
 * @see    `icaruscode/PMT/Trigger/Utilities/PlotSandbox.h`
 */

#ifndef ICARUSALG_UTILITIES_PLOTSANDBOX_H
# error("PlotSandbox.txx should not be directly included; include 'icarusalg/Utilities/PlotSandbox.h' instead")
#endif

// ICARUS libraries
#include "icarusalg/Utilities/ROOTutils.h" // ::util::ROOT::TDirectoryChanger

// LArSoft libraries
#include "larcorealg/CoreUtils/span.h" // util::make_transformed_span(), ...
#include "larcorealg/CoreUtils/values.h"

// framework libraries
#include "cetlib_except/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h" // MF_XXX() macros

// ROOT libraries
#include "TDirectoryFile.h"
#include "TList.h"
#include "TKey.h"
#include "TClass.h"
#include "TObject.h"

// C/C++ standard libraries
#include <string_view>
#include <string>
#include <vector>
#include <iterator> // std::prev()
#include <utility> // std::forward(), std::move()
#include <type_traits> // std::add_const_t<>


//------------------------------------------------------------------------------
//---  implementation detail declarations
//------------------------------------------------------------------------------
namespace icarus::ns::util::details {
  
  // TODO it seems make_transformed_span() does not support nesting properly;
  //      this should be fixed
  template <typename Map>
  struct map_dereferenced_values_impl {
    
    static constexpr std::size_t NElement = 1U; // this is the value
    
    template <typename T>
    static constexpr decltype(auto) iterate(T&& coll) noexcept
      {
        auto extractor = [](auto&& value) -> decltype(auto)
          { return *std::get<NElement>(value); };
        return ::util::make_transformed_span(coll, extractor);
      }
    
  }; // map_values_impl
  
  template <typename Map>
  decltype(auto) map_dereferenced_values(Map&& map)
    { return map_dereferenced_values_impl<std::decay_t<Map>>::iterate(std::forward<Map>(map)); }
  
  
  /**
   * @brief Helper with management of a `TDirectory` content.
   * @tparam Backend type of class providing object management in the directory
   * 
   * This class attempts to expose both ROOT `TDirectory` and
   * `art::TFileDirectory` functionality, with focus on the latter.
   * In particular, it should be able to manage object in a ROOT-friendly way
   * and framework-friendly way.
   * 
   * The core functionality that `art::TFileDirectory` is missing is the
   * awareness and access to the ROOT directory it manages.
   * In addition, this helper also attempts to provide a simplified
   * functionality like `art::TFileDirectory` in non-_art_ contexts.
   * 
   * The "backend" is the object providing additional functionality: it is
   * `art::TFileDirectory` in _art_ contexts, and plain `TDirectory*` in pure
   * ROOT contexts.
   * 
   * The object exposes the part of the interface of the backend relevant to
   * `PlotSandbox`, and a `getDirectory()` to gain direct access to the managed
   * `TDirectory`.
   * 
   * @note The non-_art_ context is currently written as specialization on
   *       `TDirectory*` backend: an instantiation on a `TDirectory`-derivate
   *       class (e.g. `TDirectoryFile*`) will attempt to use the "generic"
   *       (actually, `art::TFileDirectory`-based) implementation, and will fail
   *       accordingly.
   */
  template <typename Backend>
  class TDirectoryHelper {
    
      public:
    using Backend_t = Backend;
    using DirectoryHelper_t = TDirectoryHelper<Backend_t>;
    
    TDirectoryHelper(Backend_t dir, TDirectory* ROOTdir);
    
    /// Returns the encapsulated ROOT directory.
    TDirectory* getDirectory() const { return fROOTdir; }
    
    /// Returns a copy of the backend object of this helper.
    Backend_t backend() const { return fBackend; }
    
    template <typename Obj, typename... Args>
    Obj* makeAndRegister(
      std::string const& processedName, std::string const& processedTitle,
      Args&&... args
      ) const;
    
    
    template <typename RootDir = TDirectoryFile>
    DirectoryHelper_t mkdir
      (std::string const& subdir, std::string const& dirTitle = "") const
      { return create<RootDir>(fBackend, subdir, dirTitle); }
    
    /// Creates a helper managing a subdirectory of `parentDir`.
    template <typename RootDir = TDirectoryFile>
    static DirectoryHelper_t create(
      Backend_t parentDir,
      std::string const& subdir, std::string const& dirTitle = ""
      );
    
    static DirectoryHelper_t create(Backend_t dir);
    
      private:
    
    Backend_t fBackend; ///< Directory manager.
    TDirectory* fROOTdir; ///< The managed ROOT directory.
    
  }; // class TDirectoryHelper

} // namespace icarus::ns::util::details


//------------------------------------------------------------------------------
//--- template implementation
//------------------------------------------------------------------------------
//--- icarus::ns::util::PlotSandbox::TFileDirectoryHelper
//------------------------------------------------------------------------------
template <typename Backend>
icarus::ns::util::details::TDirectoryHelper<Backend>::TDirectoryHelper
  (Backend_t dir, TDirectory* ROOTdir)
  : fBackend(dir), fROOTdir(ROOTdir)
{}


//------------------------------------------------------------------------------
template <typename Backend>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::details::TDirectoryHelper<Backend>::makeAndRegister(
  std::string const& processedName, std::string const& processedTitle,
  Args&&... args
  ) const
{
  // this is simple: we directly use backend functionality
  return fBackend.template makeAndRegister<Obj>
    (processedName, processedTitle, std::forward<Args>(args)...);
} // icarus::ns::util::details::TDirectoryHelper<>::makeAndRegister()


template <>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::details::TDirectoryHelper<TDirectory*>::makeAndRegister(
  std::string const& processedName, std::string const& processedTitle,
  Args&&... args
  ) const
{
  // the backend (TDirectory) doesn't have full make-and-register functionality;
  // we neeed to make something up... this presents a non-negligible similarity
  // with `art::TFileDirectory::makeAndRegister()`.
  
  ::util::ROOT::TDirectoryChanger DirGuard(fROOTdir);
  // use parentheses to avoid annoyance with narrowing warnings; should we?
  Obj* obj = new Obj(std::forward<Args>(args)...);
  obj->SetName(processedName.c_str());
  obj->SetTitle(processedTitle.c_str());
  fROOTdir->Append(obj); // ... key point here: register the new object
  return obj;
} // icarus::ns::util::details::TDirectoryHelper<TDirectory*>::makeAndRegister()


//------------------------------------------------------------------------------
template <typename Backend>
template <typename RootDir /* = TDirectoryFile */>
auto icarus::ns::util::details::TDirectoryHelper<Backend>::create(
  Backend_t parentDir,
  std::string const& subdir, std::string const& dirTitle /* = "" */
  )
  -> DirectoryHelper_t
{
  // NOTE: we only support a direct subdirectory of parentDir.
  // NOTE: if the directory already exists the results are undefined;
  //       we can't figure out if the direct
  
  // first we create the directory directly via ROOT,
  // but starting from the directory stored in `parentDir`
  TDirectory* pROOTdir
    = parentDir.template make<RootDir>(subdir.c_str(), dirTitle.c_str());
  
  // then we create a `art::TFileDirectory` for the same directory;
  // this is the only way we can create a new `art::TFileDirectory`,
  // and it actually does not create the ROOT directory because it's lazy,
  // and it will create it only when it is needed, if not present yet.
  return { parentDir.mkdir(subdir, dirTitle), pROOTdir };
  
} // icarus::ns::util::PlotSandbox::TFileDirectoryHelper::create()


template <>
template <typename RootDir /* = TDirectoryFile */>
auto icarus::ns::util::details::TDirectoryHelper<TDirectory*>::create(
  TDirectory* parentDir,
  std::string const& subdir, std::string const& dirTitle /* = "" */
  )
  -> DirectoryHelper_t
{
  // NOTE: we only support a direct subdirectory of parentDir.
  // NOTE: if the directory already exists the results are undefined;
  //       we can't figure out if the direct
  assert(parentDir);
  
  // we create the directory directly via ROOT,
  // starting from the directory stored in `parentDir`
  parentDir->mkdir(subdir.c_str(), dirTitle.c_str());
  // mkdir() returns the first level of TDirectory that was created;
  // we want the last here (despite the disclaimer above):
  TDirectory* pROOTdir = parentDir->GetDirectory(subdir.c_str());
  if (!pROOTdir) {
    throw cet::exception("PlotSandbox")
      << "TDirectoryHelper::create() failed to create '" << subdir
      << "' subdirectory of '" << parentDir->GetPath() << "'!\n";
  }
  
  return { pROOTdir, pROOTdir };
  
} // icarus::ns::util::details::TDirectoryHelper<TDirectory*>::create()


//------------------------------------------------------------------------------
template <typename Backend>
auto icarus::ns::util::details::TDirectoryHelper<Backend>::create
  (Backend_t dir) -> DirectoryHelper_t
{
  
  /*
   * Finding the ROOT directory is going to be tricky, since
   * `art::TFileDirectory` includes precious `mkdir()`, `make()` and
   * `makeAndRegister()`, and that's it.
   * 
   * So the plan is:
   * 1. have `art::TFileDirectory` `make()` a new ROOT object in its directory;
   *    let that ROOT object be a `TDirectory`
   * 2. `TDirectory` is placed by default in the current directory (as usual),
   *    and it knows which its parent directory is
   * 3. after learning which that parent directory is, we delete the directory
   *    we just created; this also updates the mother directory
   */
  
  static constexpr const char* TestDirName = " PlotSandbox invalid name! ";
  
  // even if another directory with this name existed, it would not be replaced,
  // but rather another one would be created with this name:
  TDirectory* testDir = dir.template make<TDirectory>(TestDirName, TestDirName);
  if (!testDir) {
    throw cet::exception("PlotSandbox") << "TFileDirectoryHelper::create() "
      "failed to figure out the ROOT directory!\n";
  }
  TDirectory* pROOTdir = testDir->GetMotherDir();
  MF_LOG_DEBUG("TFileDirectoryHelper")
    << "icarus::ns::util::PlotSandbox::TFileDirectoryHelper::create(): "
    << "found parent directory: '" << pROOTdir->GetName() << "'";
  // ... and even if there are multiple directories with the same name, using
  // the pointer to the object makes this deletion affect only the object itself
  delete testDir;
  
  return { dir, pROOTdir };
} // icarus::ns::util::details::TFileDirectoryHelper::create()


/*
 * The general template uses tricks for `art::TFileDirectory`;
 * this one is way simpler than that...
 */
template <>
auto icarus::ns::util::details::TDirectoryHelper<TDirectory*>::create
  (TDirectory* dir) -> DirectoryHelper_t
{
  return { dir, dir };
} // icarus::ns::util::details::TFileDirectoryHelper::create()



//------------------------------------------------------------------------------
//--- icarus::ns::util::PlotSandbox
//------------------------------------------------------------------------------
template <typename DirectoryBackend>
void icarus::ns::util::PlotSandbox<DirectoryBackend>::Data_t::resetSubboxParents
  (PlotSandbox_t const* newParent)
{
  for (auto& subbox: ::util::values(subBoxes)) subbox->setParent(newParent);
} // icarus::ns::util::PlotSandbox::Data_t::resetSubboxParents()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename SandboxType>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::findSandbox
  (SandboxType& sandbox, std::string const& name) -> PlotSandbox_t*
{
  // if `name` does not have any path, `firstDir` is empty
  auto const& [ firstDir, restOfPath ] = peelDir(name);
  
  // dirName denotes the first sandbox in the path
  std::string const& dirName = firstDir.empty()? restOfPath: firstDir;
  
  auto const it = sandbox.fData.subBoxes.find(dirName);
  PlotSandbox_t* dir
    = (it == sandbox.fData.subBoxes.end())? nullptr: it->second.get();
  
  // if there is still path to search, recurse
  return (!dir || firstDir.empty())? dir: findSandbox(*dir, restOfPath);
} // icarus::ns::util::PlotSandbox::findSandbox()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename SandboxType>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::demandSandbox
  (SandboxType& sandbox, std::string const& name) -> PlotSandbox_t&
{
  auto* box = findSandbox(sandbox, name);
  if (box) return *box;
  
  cet::exception e { "PlotSandbox" };
  e << "PlotSandbox::demandSandbox(): box '" << name
    << "' not available in the sandbox '" << sandbox.ID() << "'";
  if (sandbox.nSubSandboxes()) {
    e << "\n" << "Available nested boxes (" << sandbox.nSubSandboxes() << "):";
    for (auto const& subbox: sandbox.subSandboxes())
      e << "\n * '" << subbox.ID() << "'";
  } // if has subboxes
  else {
    e << "  (no contained box!)";
  }
  throw e << "\n";

} // icarus::ns::util::PlotSandbox::demandSandbox(SandboxType)


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
icarus::ns::util::PlotSandbox<DirectoryBackend>::PlotSandbox(
  DirectoryBackend parentDir,
  std::string name, std::string desc
  )
  : fData {
    std::move(name), std::move(desc),
    name.empty()
      ? DirectoryHelper_t::create(parentDir)
      : DirectoryHelper_t::create(parentDir, name, desc)
    }
{}


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
icarus::ns::util::PlotSandbox<DirectoryBackend>::PlotSandbox
  (PlotSandbox_t&& from)
  : fData(std::move(from.fData))
{
  fData.resetSubboxParents(this); // adjust the parent pointers of the subboxes
} // PlotSandbox::PlotSandbox(PlotSandbox&&)


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string icarus::ns::util::PlotSandbox<DirectoryBackend>::ID() const
  { return fData.parent? (fData.parent->ID() + '/' + name()): name(); }


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string icarus::ns::util::PlotSandbox<DirectoryBackend>::processName
  (std::string const& name) const
{
  std::string const sandboxName = processedSandboxName();
  return sandboxName.empty()? name: name + '_' + sandboxName;
} // icarus::ns::util::PlotSandbox::processName()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string icarus::ns::util::PlotSandbox<DirectoryBackend>::processTitle
  (std::string const& title) const
{
  std::string const sandboxDesc = processedSandboxDesc();
  return sandboxDesc.empty()? title: title + ' ' + sandboxDesc;
} // icarus::ns::util::PlotSandbox::processTitle()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
bool icarus::ns::util::PlotSandbox<DirectoryBackend>::empty() const {
  
  std::vector<TDirectory const*> subDirectories; // directories of subboxes
  
  // if any of the subboxes is not empty, then this one is not either
  for (auto& subbox: subSandboxes()) {
    if (!subbox.empty()) return false;
    subDirectories.push_back(subbox.getDirectory());
  } // for
  
  auto const isSubDirectory = [b=subDirectories.begin(),e=subDirectories.end()]
    (TObject const* obj)
    {
      auto dir = dynamic_cast<TDirectory const*>(obj);
      return dir && std::find(b, e, dir) != e;
    };
  
  // if there is any object in memory associated to the directory,
  // that is not empty (directories from subboxes are exempted)
  for (TObject const* obj: *(getDirectory()->GetList()))
    if (!isSubDirectory(obj)) return false;
  
  // if there is any key associated to the directory, that is not empty
  if (getDirectory()->GetListOfKeys()->GetSize()) return false;
  
  return true;
} // icarus::ns::util::PlotSandbox::empty()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj /* = TObject */>
Obj const* icarus::ns::util::PlotSandbox<DirectoryBackend>::get
  (std::string const& name) const
  { return use<std::add_const_t<Obj>>(name); }


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj /* = TObject */>
Obj* icarus::ns::util::PlotSandbox<DirectoryBackend>::use(std::string const& name) const {
  
  auto [ objDir, objName ] = splitPath(name);
  
  TDirectory* dir = getDirectory(objDir);
  if (!dir) return nullptr;
  
  std::string const processedName = processName(objName);
  return dir->Get<Obj>(processedName.c_str());
  
} // icarus::ns::util::PlotSandbox::use()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj /* = TObject */>
Obj& icarus::ns::util::PlotSandbox<DirectoryBackend>::demand
  (std::string const& name) const
{
  
  auto* obj = use<Obj>(name);
  if (obj) return *obj;
  cet::exception e { "PlotSandbox" };
  e << "PlotSandbox::demand(): object '" << name
    << "' not available in the sandbox '" << ID() << "'"
    << "\nBox content: ";
  dumpContent(e, "", ""); // no indent
  
  throw e << "\n";
} // icarus::ns::util::PlotSandbox::demand()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename DirObj /* = TDirectory */>
DirObj* icarus::ns::util::PlotSandbox<DirectoryBackend>::getDirectory() const
  { return dynamic_cast<DirObj*>(fData.outputDir.getDirectory()); }


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename DirObj /* = TDirectory */>
DirObj* icarus::ns::util::PlotSandbox<DirectoryBackend>::getDirectory
  (std::string const& path) const
{
  TDirectory* pBaseDir = fData.outputDir.getDirectory();
  return dynamic_cast<DirObj*>
    (path.empty()? pBaseDir: pBaseDir->GetDirectory(path.c_str()));
} // icarus::ns::util::PlotSandbox::getDirectory()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::PlotSandbox<DirectoryBackend>::make
  (std::string const& name, std::string const& title, Args&&... args)
{
  auto [ objDir, objName ] = splitPath(name);
  
  std::string const processedName = processName(objName);
  std::string const processedTitle = processPlotTitle(title);
  
  DirectoryHelper_t destDir // no title for the implicit subdirectories
    = objDir.empty()? fData.outputDir: fData.outputDir.mkdir(objDir);
  
  return makeImpl<Obj>
    (destDir, processedName, processedTitle, std::forward<Args>(args)...);
  
} // icarus::ns::util::PlotSandbox::make()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::PlotSandbox<DirectoryBackend>::acquire(
  std::unique_ptr<Obj>&& obj,
  std::string const& name /* = "" */, std::string const& title /* = "" */
  )
{
  std::string const& newName = name.empty()? obj->GetName(): name;
  std::string const& newTitle = title.empty()? obj->GetTitle(): title;
  // object will be deleted, but its content will have been moved away already
  std::unique_ptr<Obj> local{ std::move(obj) };
  // problem: ROOT objects don't move that well
  return make<Obj>(newName, newTitle, NoNameTitle, std::move(*obj));
} // icarus::ns::util::PlotSandbox::acquire()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template
  <typename SandboxType /* = icarus::ns::util::PlotSandbox */, typename... Args>
SandboxType& icarus::ns::util::PlotSandbox<DirectoryBackend>::addSubSandbox
  (std::string const& name, std::string const& desc, Args&&... args)
{
  // deal with sandbox paths via recursion
  auto [ dir, baseName ] = splitPath(name);
  if (!dir.empty()) {
    try {
      return demandSandbox(dir).template addSubSandbox<SandboxType>
        (baseName, desc, std::forward<Args>(args)...);
    }
    catch (cet::exception const& e) {
      throw cet::exception("PlotSandbox", "", e)
        << "PlotSandbox::addSubSandbox(): failed to add a subbox with name '"
        << name << "' in  box '" << ID() << "'.\n";
    }
  }
  
  // we can't use make_unique() because the constructor it needs is protected:
  auto [ it, bInserted ] = fData.subBoxes.try_emplace(
    baseName,
    new SandboxType(*this, baseName, desc, std::forward<Args>(args)...)
    );
  if (!bInserted) {
    throw cet::exception("PlotSandbox")
      << "PlotSandbox::addSubSandbox(): a subbox with name '" << baseName
      << "' already exists in  box '" << ID() << "'.\n";
  }
  return *(it->second); // it iterator to the inserted element
} // icarus::ns::util::PlotSandbox::addSubSandbox()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::findSandbox
  (std::string const& name) -> PlotSandbox_t*
  { return findSandbox(*this, name); }

template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::findSandbox
  (std::string const& name) const -> PlotSandbox_t const*
  { return findSandbox(*this, name); }


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::demandSandbox
  (std::string const& name) -> PlotSandbox_t&
  { return demandSandbox(*this, name); }

template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::demandSandbox
  (std::string const& name) const -> PlotSandbox_t const&
  { return demandSandbox(*this, name); }


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
bool icarus::ns::util::PlotSandbox<DirectoryBackend>::deleteSubSandbox
  (std::string const& name)
{
  // deal with sandbox paths via recursion
  if (auto [ dir, baseName ] = splitPath(name); !dir.empty()) {
    auto* parent = findSandbox(dir);
    return parent? parent->deleteSubSandbox(baseName): false;
  }
  
  auto const it = fData.subBoxes.find(name);
  if (it == fData.subBoxes.end()) return false;
  
  if (it->second) {
    auto&& subbox = std::move(it->second); // will get destroyed at end of scope
    if (subbox->getDirectory()) delete subbox->getDirectory();
    if (getDirectory()) getDirectory()->Delete((name + ";*").c_str());
  }
  
  fData.subBoxes.erase(it);
  return true;
} // icarus::ns::util::PlotSandbox::deleteSubSandbox()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Stream>
void icarus::ns::util::PlotSandbox<DirectoryBackend>::dump
  (Stream&& out, std::string indent, std::string firstIndent) const
{
  out << firstIndent;
  if (hasName()) out << "Box '" << name() << "'";
  else           out << "Unnamed box";
  if (hasDescription()) out << " (\"" << description() << "\")";
  out << " [ID=" << ID() << "] with ";
  dumpContent(std::forward<Stream>(out), indent, "");
  
  if (nSubSandboxes()) {
    out << "\n" << indent << "Nested boxes (" << nSubSandboxes() << "):";
    for (auto const& subbox: subSandboxes()) {
      out << "\n";
      subbox.dump(std::forward<Stream>(out), indent + "  ");
    }
  } // if has subboxes
} // icarus::ns::util::PlotSandbox::dump()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
icarus::ns::util::PlotSandbox<DirectoryBackend>::PlotSandbox
  (PlotSandbox_t const& parent, std::string name, std::string desc)
  : PlotSandbox
    { parent.fData.outputDir.backend(), std::move(name), std::move(desc) }
{
  setParent(&parent);
}


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string icarus::ns::util::PlotSandbox<DirectoryBackend>::processPlotTitle
  (std::string const& title) const
{
  /*
   * We want to process the title of the plot, but this "title" string might
   * contain also axis labels (see e.g. `TH1::SetTitle()`).
   * 
   * We need to first identify the actual title portion of the "title",
   * then process it alone and merge it back (one Python line...).
   * 
   */
  
  // eat title until an unescaped ';' is found
  auto const tbegin = title.begin();
  auto const tend = title.end();
  auto atend = tbegin; // actual title end
  while (atend != tend) {
    if ((*atend == ';') && ((atend == tbegin) || (*std::prev(atend) != '\\')))
      break;
    ++atend;
  } // while
  
  return processTitle({ tbegin, atend }).append(atend, tend);
  
} // icarus::ns::util::PlotSandbox::processedPlotTitle()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string
icarus::ns::util::PlotSandbox<DirectoryBackend>::processedSandboxName() const
{
  std::string processed;
  
  // if there is no name, the processed name is empty (we don't recurse parents)
  if (!hasName()) return processed;
  
  processed = name();
  if (fData.parent) processed += '_' + fData.parent->processedSandboxName();
  
  return processed;
} // icarus::ns::util::PlotSandbox::processedSandboxName()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string
icarus::ns::util::PlotSandbox<DirectoryBackend>::processedSandboxDesc() const
{
  std::string processed;
  
  // if there is no name, the processed name is empty (we don't recurse parents)
  if (hasDescription()) processed += description();
  
  if (fData.parent) processed += ' ' + fData.parent->processedSandboxDesc();
  
  return processed;
} // icarus::ns::util::PlotSandbox::processedSandboxDesc()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Stream>
void icarus::ns::util::PlotSandbox<DirectoryBackend>::dumpContent
  (Stream&& out, std::string indent, std::string firstIndent) const
{
  out << firstIndent;
  
  TDirectory const* pDir = fData.outputDir.getDirectory();
  if (!pDir) {
    out << "no content available";
    return;
  }
  
  TList const* objects = pDir->GetList();
  TList const* keys = pDir->GetListOfKeys();
  if (objects && !objects->IsEmpty()) {
    out << objects->GetSize() << " direct entries:";
    for (TObject const* obj: *objects) {
      out << "\n" << indent << "  '" << obj->GetName() << "'  ["
        << obj->IsA()->GetName() << "]";
    } // for objects
  }
  else out << "no direct entries;";
  if (keys) {
    for (TObject const* keyObj: *keys) {
      auto key = dynamic_cast<TKey const*>(keyObj);
      if (!key) continue;
      if (objects->Contains(key->GetName())) continue; // already in objects
      out << "\n" << indent
        << "[KEY]  '" << key->GetName() << "'  ["
        << key->GetClassName() << "]"
        ;
    } // for
  } // if has keys
  
} // icarus::ns::util::PlotSandbox::dumpContent()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::peelDir
  (std::string const& path, char sep /* = '/' */)
  -> std::pair<std::string, std::string>
{
  
  auto const iSep = path.find(sep);
  if (iSep == std::string::npos) return { {}, path };
  else return { path.substr(0U, iSep), path.substr(iSep + 1) };
  
} // icarus::ns::util::PlotSandbox::peelDir()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
auto icarus::ns::util::PlotSandbox<DirectoryBackend>::splitPath
  (std::string const& path, char sep /* = '/' */)
  -> std::pair<std::string, std::string>
{
  
  auto const iSep = path.rfind(sep);
  if (iSep == std::string::npos) return { {}, path };
  else return { path.substr(0U, iSep), path.substr(iSep + 1) };
  
} // icarus::ns::util::PlotSandbox::splitPath()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
std::string icarus::ns::util::PlotSandbox<DirectoryBackend>::joinPath
  (std::initializer_list<std::string> pathElements, char sep /* = '/' */)
{
  if (std::empty(pathElements)) return {};
  
  auto stripSep = [sep](std::string const& s) -> std::string_view {
    return {
      s.data(),
      ((s.length() > 1) && (s.back() == sep))? s.length() - 1: s.length()
      };
    };
  
  auto iElem = pathElements.begin();
  auto const eend = pathElements.end();
  auto const& first = stripSep(*iElem);
  std::string s { first.begin(), first.end() };
  while (++iElem != eend) {
    auto const& elem = stripSep(*iElem);
    if (elem.empty()) continue;
    if (elem.front() != sep) s.push_back(sep);
    s += elem;
  } // while
  return s;
} // icarus::ns::util::PlotSandbox::joinPath()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::PlotSandbox<DirectoryBackend>::makeImpl(
  DirectoryHelper_t destDir,
  std::string const& processedName, std::string const& processedTitle,
  Args&&... args
) const {
  
  return destDir.template makeAndRegister<Obj>(
    processedName, processedTitle,
    processedName.c_str(), processedTitle.c_str(), std::forward<Args>(args)...
    );
} // icarus::ns::util::PlotSandbox::makeImpl()


//------------------------------------------------------------------------------
template <typename DirectoryBackend>
template <typename Obj, typename... Args>
Obj* icarus::ns::util::PlotSandbox<DirectoryBackend>::makeImpl(
  DirectoryHelper_t destDir,
  std::string const& processedName, std::string const& processedTitle,
  NoNameTitle_t, Args&&... args
) const {
  
  return destDir.template makeAndRegister<Obj>
    (processedName, processedTitle, std::forward<Args>(args)...);
} // icarus::ns::util::PlotSandbox::makeImpl()


//------------------------------------------------------------------------------
