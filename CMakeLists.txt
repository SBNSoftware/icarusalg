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


cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
# cmake_policy(VERSION 3.19)

find_package(cetmodules 3.20.00 REQUIRED)
project(icarusalg VERSION 09.85.00 LANGUAGES CXX)

message(STATUS
  "\n-- ============================================================================="
  "\n-- === *** Start of configuration of ${PROJECT_NAME} ***"
  "\n-- ===")


include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic
)

cet_report_compiler_flags(REPORT_THRESHOLD VERBOSE)

# these are minimum required versions, not the actual product versions
find_package( sbnobj        REQUIRED EXPORT )
find_package( larcorealg    REQUIRED EXPORT )
find_package( larcoreobj    REQUIRED EXPORT )
find_package( lardataalg    REQUIRED EXPORT )
find_package( lardataobj    REQUIRED EXPORT )
find_package( nusimdata     REQUIRED )
find_package( dk2nudata     REQUIRED )
find_package( cetlib        REQUIRED EXPORT )
find_package( cetlib_except REQUIRED EXPORT )
find_package( canvas        REQUIRED EXPORT )
find_package( canvas_root_io )
find_package( fhiclcpp      REQUIRED EXPORT )
find_package( messagefacility )
find_package( ROOT          REQUIRED EXPORT )
find_package( Boost COMPONENTS unit_test_framework)
find_package( CLHEP         REQUIRED EXPORT )
find_package( Microsoft.GSL HINTS $ENV{GUIDELINE_SL_DIR} REQUIRED EXPORT )

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

# packaging utility

# detect canvas version and tell it to the software
string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" CANVAS_VERSION_LIST "${canvas_VERSION}")
list(GET CANVAS_VERSION_LIST 0 CANVAS_MAJOR_VERSION)
list(GET CANVAS_VERSION_LIST 1 CANVAS_MINOR_VERSION)
list(GET CANVAS_VERSION_LIST 2 CANVAS_PATCH_VERSION)
math(EXPR CANVAS_DEC_VERSION
  "${CANVAS_MAJOR_VERSION} * 10000 + ${CANVAS_MINOR_VERSION} * 100 + ${CANVAS_PATCH_VERSION}"
  OUTPUT_FORMAT DECIMAL
  )
add_definitions("-DCANVAS_DEC_VERSION=${CANVAS_DEC_VERSION}")
message(STATUS "Detected canvas version ${CANVAS_MAJOR_VERSION}.${CANVAS_MINOR_VERSION}.${CANVAS_PATCH_VERSION}'.")

message(STATUS
  "\n-- ==="
  "\n-- === *** End of configuration of ${PROJECT_NAME} ***"
  "\n-- ============================================================================="
  "\n-- "
  )

cet_cmake_config()
