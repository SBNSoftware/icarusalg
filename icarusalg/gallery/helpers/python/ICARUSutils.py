#!/usr/bin/env python


__doc__ = """
Collection of utilities to interface ICARUS with python, gallery and LArSoft.

This module requires ROOT.
"""

__all__ = [
  'loadICARUSgeometry',
  'loadICARUSwireReadout',
  'loadICARUSauxDetgeometry'
]

import galleryUtils
import LArSoftUtils
import ROOTutils
from ROOTutils import ROOT


################################################################################
ICARUSwireReadoutSettings = {
  'WireReadoutICARUS': {
    'wireReadoutClassName': 'icarus::WireReadoutGeomICARUS',
    'load':               [
                            'larcorealg_Geometry',
                            'icarusalg/Geometry/WireReadoutGeomICARUS.h',
                            'icarusalg/Geometry/ICARUSstandaloneGeometrySetup.h',
                            'icarusalg_Geometry',
                          ],
  }, # 'WireReadoutICARUS'
}
DefaultChannelMapping = 'WireReadoutICARUS'

################################################################################
### Geometry
###
def loadICARUSwireReadoutClass(
  config: "WireReadout service configuration (as FHiCL parameter set)",
  registry: "ServiceRegistryClass object with the configuration of all services",
  ) -> "Class object for the proper channel mapping":
  #
  # The new pattern here (changed from icaruscode v10.04.04) is:
  #  * WireReadout (art service) is an interface; the service implementation
  #    is tasked with creating a `WireReadoutGeom` object. ICARUS uses its own
  #    implementation of the service, which is called `WireReadoutICARUS`
  #  * WireReadoutICARUS creates an instance of `WireReadoutICARUS`,
  #    loading the geometry sorter to be used (for planes and wires) with an
  #    art tool. The creation is straightforward, after a `GeometryCore` and a
  #    sorter are provided.
  # 
  # The purpose of this function is to find which object needs to be used
  # (`icarus::WireReadoutGeomICARUS` in the above), load its definition and
  # return a class object (not a `icarus::WireReadoutGeomICARUS` object)
  # that will be constructed by the caller (in the above "straightforward" way).
  #

  #
  # we need to:
  # 1. find out which mapping is required
  # 2. load the proper libraries
  # 3. return the Python class object for the mapping class we want
  #

  #
  # 1. find out which mapping is required: known configurations
  #
  serviceProviderName = config.get[str]('service_provider')

  #
  # 2. load the proper libraries
  #

  # get the specification record
  try: wireReadoutSetupInfo = ICARUSwireReadoutSettings[serviceProviderName]
  except KeyError:
    # when you get to this error, check that the tool name in the configuration
    # is actually spelled correctly first...
    raise RuntimeError(
     f"Wire readout geometry service not supported: '{serviceProviderName}': Python library needs to be updated."
     )
  # try ... except

  # load the libraries
  for codeObj in wireReadoutSetupInfo.get('load', []):
    LArSoftUtils.SourceCode.load(codeObj)

  # get the class object
  try: mapperClass = ROOTutils.getROOTclass(wireReadoutSetupInfo['wireReadoutClassName'])
  except AttributeError:
    # this needs investigation, as the code above should be sufficient to it
    raise RuntimeError(
      f"The library with '{wireReadoutSetupInfo['wireReadoutClassName']}' has not been correctly loaded!"
      )
  # try ... except

  #
  # 3. return the Python class object for the mapping class we want
  #
  return mapperClass

# loadICARUSwireReadoutClass()


def loadICARUSgeometry(
  config = None, registry = None, mappingClass = None,
  ):
  """Loads and returns ICARUS geometry with the standard ICARUS sorting.

  See `loadGeometry()` for the meaning of the arguments.
  """

  SourceCode = LArSoftUtils.SourceCode # alias

  SourceCode.loadHeaderFromUPS('icarusalg/Geometry/GeoObjectSorterPMTasTPC.h')
  SourceCode.loadLibrary('icarusalg_Geometry')
  return LArSoftUtils.loadGeometry(
    config=config, registry=registry,
    sorterClass=ROOT.icarus.GeoObjectSorterPMTasTPC,
    )

# loadICARUSgeometry()


def loadICARUSwireReadout(config = None, registry = None, geometry = None):
  """Loads and returns ICARUS wire readout with the standard ICARUS channel mapping.
  
  See `loadWireReadout()` for the meaning of the arguments.
  """
  
  # we use the common `loadWireReadout()`, but before that, we need to load
  # all the classes we need and instantiate the sorter.
  assert registry or geometry, "Registry is required if no geometry is provided"

  serviceName = 'WireReadout'
  serviceConfig = config.service(serviceName) if config else registry.config(serviceName)
  
  wireReadoutClass = loadICARUSwireReadoutClass(config=serviceConfig, registry=registry)
  
  # we ignore the sorter specification from the configuration here
  DefaultSorterClassName = "WireReadoutSorterStandard"
  sorterClassName = serviceConfig.get[str]("SortingParameters.tool_type", DefaultSorterClassName)
  if sorterClassName != DefaultSorterClassName:
    # this may be as easy as just doing it...
    raise RuntimeError(f"Sorting algorithm '{sorterClassName}' not supported.")
  # try ...
  
  # whatever the sorter class is, it must have its own header named after it
  # and be defined in icarusalg_Geometry library.
  SourceCode = LArSoftUtils.SourceCode # alias
  if sorterClassName == DefaultSorterClassName:
    SourceCode.loadHeaderFromUPS(f'larcorealg/Geometry/WireReadoutSorterStandard.h')
    SourceCode.loadLibrary('larcorealg_Geometry') # should be already loaded by now
  else:
    SourceCode.loadHeaderFromUPS(f'icarusalg/Geometry/{sorterClassName}.h')
    SourceCode.loadLibrary('icarusalg_Geometry') # should be already loaded by now
  sorter = getattr(ROOT.geo, sorterClassName)
  
  return LArSoftUtils.loadWireReadout(
    config=config, registry=registry,
    sorterClass=sorter,
    serviceClass=wireReadoutClass,
    geometry=geometry,
    )
  
# loadICARUSwireReadout()


def loadICARUSauxDetgeometry(config = None, registry = None):
  """Loads and returns ICARUS geometry with the standard channel mapping.
  
  Because we can't yet figure out how to use a different mapping.
  And however ICARUS uses a separate scheme for CRT channel mapping.
  Which is in `icaruscode`.
  
  See `loadAuxDetGeometry()` for the meaning of the arguments.
  """
  SourceCode = LArSoftUtils.SourceCode # alias
  
  # SourceCode.loadHeaderFromUPS('icaruscode/CRT/CRTGeoObjectSorter.h')
  # SourceCode.loadLibrary('icaruscode_CRTData')
  # SourceCode.loadHeaderFromUPS('icaruscode/CRT/CRTAuxDetInitializerICARUS.h')
  # SourceCode.loadLibrary('libicaruscode_CRT_CRTAuxDetInitializerICARUS_tool')

  return LArSoftUtils.loadAuxDetGeometry(config=config, registry=registry)
  #  , auxDetReadoutInitClass=ROOT.icarus.crt.CRTAuxDetInitializerICARUS)

# loadICARUSauxDetgeometry()


################################################################################
