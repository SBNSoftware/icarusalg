cet_make_library(LIBRARY_NAME Test INTERFACE
  SOURCE
    FrameworkEventMockup.h
  LIBRARIES INTERFACE
    canvas::canvas
)

# cmake driver file for testing from CET build tools
include(CetTest)

# Always enable asserts (for tests only)
cet_enable_asserts()

# test directories
add_subdirectory(Geometry)
add_subdirectory(Utilities)
add_subdirectory(PMT)
add_subdirectory(python)

