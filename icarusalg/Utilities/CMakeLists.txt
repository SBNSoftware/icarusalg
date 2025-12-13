
file(GLOB src_files *.cxx)
cet_make_library(
  SOURCE ${src_files}
  LIBRARIES
    sbnobj::Common_Trigger   # BeamBits.h
    lardataalg::DetectorInfo
    lardataalg::headers
    larcorealg::CoreUtils
    lardataobj::RecoBase
    canvas::canvas
    cetlib_except::cetlib_except
    CLHEP::CLHEP
    Microsoft.GSL::GSL
  )

install_headers()
install_source()
install_fhicl()
