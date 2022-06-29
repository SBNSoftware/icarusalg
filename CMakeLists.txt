# ======================================================================
#  icaruscode main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/larsoft/ups/setup_for_development <-d|-p>
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================


cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(icarusalg VERSION 09.37.02.09 LANGUAGES CXX)

message(STATUS
  "\n-- ============================================================================="
  "\n-- === *** Start of configuration of ${PROJECT_NAME} ***"
  "\n-- ===")

# cetbuildtools contains our cmake modules
find_package(cetbuildtools REQUIRED)

list(APPEND CMAKE_MODULE_PATH $ENV{CANVAS_ROOT_IO_DIR}/Modules)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic -I $ENV{CLANG_FQ_DIR}/include/c++/v1
)

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( larcorealg )
find_ups_product( larcoreobj )
find_ups_product( lardataalg )
find_ups_product( lardataobj )
find_ups_product( nusimdata  )
find_ups_product( canvas     )
find_ups_product( cetlib     )
find_ups_product( clhep      )
find_ups_product( guideline_sl )
find_ups_root()
find_ups_boost()

# macros for dictionary and simple_plugin
include(ArtDictionary)
include(CetMake)
include(BasicPlugin)

# ADD SOURCE CODE SUBDIRECTORIES HERE
add_subdirectory(icarusalg)

# subdirectory for test code
add_subdirectory(test)

# Top level fcl files.
add_subdirectory(fcl)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)

message(STATUS
  "\n-- ==="
  "\n-- === *** End of configuration of ${PROJECT_NAME} ***"
  "\n-- ============================================================================="
  "\n-- "
  )

