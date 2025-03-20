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

set (exes dumpICARUSgeometry.py)
if(exes)
  install_scripts(LIST ${exes})
endif(exes)

install_source()
