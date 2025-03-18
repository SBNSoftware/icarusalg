# We follow the pattern of `icarusutil`/`larbatch`:
# we in principle distinguish executable and non-executable (module) python files.
# The former are installed with execute permission.
# The latter are installed without execute permission.
# In practice, everything that is not explicitly listed as executable is
# considered module.

# 
# Executable python files
# (install in a PATH-accessible directory CET-knows-where)
# 

set (exes) # none so far
if(exes)
  message(STATUS "Executable python scripts: ${exes}")
  install_scripts(LIST ${exes})
endif(exes)

#
# Non-executable python files
# (install in our custom directory, set up by a "table fragment" in ups/product_deps)
#
set (nonexes ICARUSutils.py;ICARUSservices.py)
if(nonexes AND exes)
  list(REMOVE_ITEM nonexes ${exes})
endif(nonexes AND exes)
if(nonexes)
  message(STATUS "Non-executable python modules: ${nonexes}")
  install(FILES ${nonexes} DESTINATION "python")
endif(nonexes)
