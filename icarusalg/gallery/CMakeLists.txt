# official shared software
add_subdirectory(helpers)

# examples
install(DIRECTORY "examples/" DESTINATION "examples/gallery"
  REGEX "(build|lib|bin|CMakeFiles)/.*" EXCLUDE # attempt to avoid accidental installation of CMake build stuff
  )

# other stuff
add_subdirectory(MCTruthBase)

