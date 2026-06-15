cet_make_library(
  SOURCE
    "SharedWaveformBaseline.cxx"
    "OverlayPMTwaveformAlg.cxx"
  LIBRARIES
    icarusalg::Utilities
    sbnobj::ICARUS_PMT_Data
    lardataalg::UtilitiesHeaders
    lardataobj::RawData
    messagefacility::MF_MessageLogger
    cetlib_except::cetlib_except
  )

install_headers()
install_fhicl()
install_source()
