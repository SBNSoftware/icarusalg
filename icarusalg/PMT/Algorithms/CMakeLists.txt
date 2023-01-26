cet_make_library(
  SOURCE
    "SharedWaveformBaseline.cxx"
  LIBRARIES
    lardataalg::UtilitiesHeaders
    lardataobj::RawData
    messagefacility::MF_MessageLogger
  )

install_headers()
install_fhicl()
install_source()
