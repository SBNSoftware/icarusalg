/**
 * @file   expandInputFiles.cxx
 * @brief  Function to expand file lists.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    October 19, 2017
 * @see     expandInputFiles.cxx
 * 
 * This implementation file may have been included in its header already
 * (just for convenience reasons).
 * 
 */


// library header
#include "expandInputFiles.h"

// C/C++ libraries
#include <fstream>
#include <cctype> // std::isspace()
#include <stdexcept> // std::runtime_error


// -----------------------------------------------------------------------------
namespace details {
  
  template <typename T>
  std::vector<T>& appendToVector
    (std::vector<T>& dest, std::vector<T> const& src)
    { dest.insert(dest.end(), src.begin(), src.end()); return dest; }
  
  template <typename StartIter, typename EndIter>
  StartIter skipSpaces(StartIter begin, EndIter end) {
    for (auto i = begin; i != end; ++i) if (!std::isspace(*i)) return i;
    return end;
  } // skipSpaces()
  
  
  struct FileListExpansionBaseError: public std::runtime_error
    { using std::runtime_error::runtime_error; };
  
  struct FileNotFoundError: public FileListExpansionBaseError {
    FileNotFoundError(std::string const& fileName)
      : FileListExpansionBaseError("Can't open file '" + fileName + "'")
      {}
  };
  
  struct FileListErrorWrapper: public FileListExpansionBaseError {
    FileListErrorWrapper(std::string const& fileName, unsigned int line)
      : FileListExpansionBaseError(formatMsg(fileName, line))
      {}
    FileListErrorWrapper(
      std::string const& fileName, unsigned int line,
      FileListExpansionBaseError const& error
      )
      : FileListExpansionBaseError
        (std::string(error.what()) + "\n  " + formatMsg(fileName, line))
      {}
      
      private:
    static std::string formatMsg(std::string const& fname, unsigned int line)
      { return "Error from file list '" + fname + "' line " + std::to_string(line); }
  };
  
} // namespace details


// -----------------------------------------------------------------------------
#if HAS_STD_FILESYSTEM
inline bool isROOTfile(std::filesystem::path const& filePath)
  { return (filePath.extension() == ".root") && !filePath.stem().empty(); }
#else // no STL filesystem library:
inline bool isROOTfile(std::string const& filePath) {
  if (filePath.length() < 6) return false; // too short (".root" is not good!)
  
  auto iExt = filePath.rfind('.');
  if (iExt == std::string::npos) return false; // no file extension
  
  auto iBaseName = filePath.rfind('/');
  if ((iBaseName != std::string::npos) && (iBaseName > iExt))
    return false; // still no extension
  
  return filePath.substr(iExt) == ".root";
} // isROOTfile()

#endif // STL filesystem library?


// -----------------------------------------------------------------------------
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
inline
#endif
std::vector<std::string> expandFileList(std::string const& listPath) {
  
  std::ifstream list(listPath);
  if (!list) throw details::FileNotFoundError(listPath);
  
  std::vector<std::string> files;
  std::string line;
  unsigned int iLine = 0;
  do {
    ++iLine; // currently unused
    std::getline(list, line);
    if (!list) break;
    
    auto const end = line.cend();
    
    //
    // find the start of the file name
    //
    auto i = details::skipSpaces(line.cbegin(), end);
    if (i == end) continue; // empty line
    if (*i == '#') continue; // full comment line
    
    std::string filePath;
    auto iChunk = i;
    while(i != end) {
      if (*i == '\\') {
        filePath.append(iChunk, i); iChunk = i;
        if (++i == end) break; // weird way to end a line, with a '\'
        if ((*i == '\\') || (*i == '#')) iChunk = i; // eat the backspace
        // the rest will be added with the next chuck
      }
      else if (std::isspace(*i)) {
        filePath.append(iChunk, i); iChunk = i; // before, there were no spaces
        auto const iAfter = details::skipSpaces(i, end);
        if (iAfter == end) break; // spaces, then end of line: we are done
        if (*iAfter == '#') break; // a comment starts after spaces: we are done
        i = iAfter; // these spaces are part of file name; schedule for writing
        continue;
      }
      ++i;
    } // for
    filePath.append(iChunk, i);
    if (filePath.empty()) continue;
    
    if (isROOTfile(filePath))
      files.push_back(filePath);
    else {
      try {
        details::appendToVector(files, expandFileList(filePath));
      }
      catch(details::FileListExpansionBaseError const& e) {
        throw details::FileListErrorWrapper(listPath, iLine, e);
      }
    }
    
  } while (true);
  return files;
} // expandFileList()


// -----------------------------------------------------------------------------
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
inline
#endif
std::vector<std::string> expandInputFiles
  (std::vector<std::string> const& filePaths)
{
  std::vector<std::string> expanded;
  for (std::string const& path: filePaths) {
    if (isROOTfile(path))
      expanded.push_back(path);
    else 
      details::appendToVector(expanded, expandFileList(path));
  } // for
  return expanded;
} // expandInputFiles()


// -----------------------------------------------------------------------------
