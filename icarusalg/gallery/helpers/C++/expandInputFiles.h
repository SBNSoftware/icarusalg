/**
 * @file   icarusalg/gallery/helpers/C++/expandInputFiles.h
 * @brief  Function to expand file lists.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    October 19, 2017
 * 
 * Defining `ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION`
 * will include the implementation of all the functions declared here inline
 * (only for convenience reasons).
 * 
 */

#ifndef ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_H
#define ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_H

// C/C++ libraries
// <filesystem> is present from GCC 8 and CLANG 6, and as experimental from GCC 6;
// but linking to these is still a mess, so until we get word from the compiler
// that <filesystem> is available, we just don't use any (not even experimental)
#if __cpp_lib_filesystem
# define HAS_STD_FILESYSTEM 1
# include <filesystem> // std::filesystem::path
#else
# define HAS_STD_FILESYSTEM 0
#endif

#include <vector>
#include <string>
#include <stdexcept> // std::runtime_error


// -----------------------------------------------------------------------------
/// Returns whether the specified path represents a file list.
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
inline
#endif
#if HAS_STD_FILESYSTEM
bool isROOTfile(std::filesystem::path const& filePath);
#else // no STL filesystem library:
bool isROOTfile(std::string const& filePath);
#endif // STL filesystem library?


// -----------------------------------------------------------------------------
/// Expands the content of a file list into a vector of file paths (recursive).
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
inline
#endif
std::vector<std::string> expandFileList(std::string const& listPath);

// -----------------------------------------------------------------------------
/// Expands all input files into a vector of file paths.
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
inline
#endif
std::vector<std::string> expandInputFiles
  (std::vector<std::string> const& filePaths);


// -----------------------------------------------------------------------------
#if ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION
# include "expandInputFiles.cxx"
#endif // ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_INLINE_IMPLEMENTATION


#endif // ICARUSALG_GALLERY_HELPERS_Cxx_EXPANDINPUTFILES_H
