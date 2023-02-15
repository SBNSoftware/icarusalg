################################################################################
cmake_minimum_required(VERSION 3.24)

# The presence of cetmodules appears to allow for some tricky UPS packages
# (e.g. `range` library) canvas depends on, which are not distributed
# with a package config script (neither pkgconfig or CMake flavor).
# Many other UPS packages appear they could work without it
# (hard to say since the dependency on `range` via `canvas` is quite hard)
find_package(cetmodules 3.20.00 REQUIRED)

#
# making `cmake` aware of Clang may require some care;
# we pick `gallery` qualifiers and if they contain `cN` then it's Clang;
# TODO check if this is still necessary with CMake 3.24
# 
if ( $ENV{SETUP_GALLERY} MATCHES "-q +([^ ]+)( |$)" )
  set ( GalleryQualifiers ${CMAKE_MATCH_1} )
  if( ${GalleryQualifiers} MATCHES "(^|:)c[0-9]+(:|$)" )
    # make cmake acknowledge the difference between Apple Clang and standard Clang:
    cmake_policy(SET "CMP0025" "NEW")
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
  else( ${GalleryQualifiers} MATCHES "(^|:)c[0-9]+(:|$)" )
    # don't bother, standard configuration will hopefully do
  endif( ${GalleryQualifiers} MATCHES "(^|:)c[0-9]+(:|$)" )
endif()

# make sure we know where to install things
if(DEFINED $ENV{GALLERY_INSTALL})
  set(GalleryInstallDir "$ENV{GALLERY_INSTALL}/" )
else(DEFINED $ENV{GALLERY_INSTALL})
  set(GalleryInstallDir "./" )
endif(DEFINED $ENV{GALLERY_INSTALL})

#
# declare the name of our project
#
project(DrawPMTwaveforms
  LANGUAGES CXX
  )

# this does not seem to affect the quality of the compilation,
# but it cuts a lot of the messages from CMake (and maybe it caches results?)
include(CetCMakeEnv)
cet_cmake_env()

# standard CMake voodoo to push the installation directory in the right direction
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE PATH "Install in source code dir" FORCE
  )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


# most packages are looked for in CMAKE_PREFIX_PATH (UPS will populate it)
find_package(gallery         REQUIRED)
find_package(icarusalg       REQUIRED)
find_package(lardataalg      REQUIRED)
find_package(lardataobj      REQUIRED)
find_package(sbnobj          REQUIRED)
find_package(messagefacility  REQUIRED)
find_package(ROOT
  COMPONENTS Gpad Graf Hist Tree RIO
  REQUIRED
  )


################################################################################
###  EXECUTABLE DIRECTIVES
################################################################################
add_executable(DrawPMTwaveforms "DrawPMTwaveforms.cpp")
target_compile_features(DrawPMTwaveforms PUBLIC cxx_std_17)
target_link_libraries(DrawPMTwaveforms
  gallery::gallery
  icarusalg::gallery_helpers
  icarusalg::Utilities
  lardataalg::headers
  lardataobj::RawData
  messagefacility::MF_MessageLogger
  ROOT::Gpad
  ROOT::Graf
  ROOT::Hist
  ROOT::RIO
  )
install(TARGETS DrawPMTwaveforms)


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
