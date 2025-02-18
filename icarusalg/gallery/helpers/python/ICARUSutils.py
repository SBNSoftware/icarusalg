#!/usr/bin/env python


__doc__ = """
Collection of utilities to interface ICARUS with python, gallery and LArSoft.

This module requires ROOT.
"""

__all__ = [
  'loadICARUSgeometry',
  'justLoadICARUSgeometry',
  'loadICARUSwireReadout',
  'justLoadICARUSwireReadout',
]

import galleryUtils
import LArSoftUtils
import ROOTutils
from ROOTutils import ROOT


################################################################################
ICARUSchannelMappings = {
  'GeoObjectSorterPMTasTPC': {
    'mapperClassName': 'icarus::GeoObjectSorterPMTasTPC',
    'load':          [
                       'larcorealg_Geometry',
                       'icarusalg/Geometry/GeoObjectSorterPMTasTPC.h',
                       'icarusalg_Geometry',
                     ],
  },
  'WireReadoutSorterStandard': {
    'mapperClassName': 'geo::WireReadoutSorterStandard',
    'load':          [
                       'larcorealg/Geometry/WireReadoutSorterStandard.h',
                       'larcorealg_Geometry',
                       'icarusalg/Geometry/GeoObjectSorterPMTasTPC.h',
                       'icarusalg_Geometry',
                     ],
  },
  'AuxDetGeoObjectSorterICARUS': {
    'mapperClassName': 'geo::AuxDetGeoObjectSorterICARUS',
    'load':          [
                       'icarusalg/Geometry/AuxDetGeoObjectSorterICARUS.h',
                       'icarusalg_Geometry',
                     ],
  },
  'CRTAuxDetInitializer': {
    'mapperClassName': 'icarus::crt::CRTAuxDetInitializerICARUS',
    'load':          [
                       'icarusalg/Geometry/CRTAuxDetInitializerICARUS.h',
                       'icarusalg_Geometry',
                     ],
  },
}
DefaultChannelMapping = 'GeoObjectSorterPMTasTPC'

################################################################################
### Geometry
###
def getConfiguration(
  config: "ConfigurationClass object with complete job configuration",
  registry: "ServiceRegistryClass object with the configuration of all services",
  serviceName: "The service name",
  ) -> "configuration of channel mapping algorithm as a FHiCL parameter set":

  #
  # Try first if there is a configuration in the geometry service configuration;
  # this is the "default" for the future. If not, back up to
  # ExptGeoHelperInterface service.
  #

  # serviceName = 'Geometry'
  # serviceName = 'WireReadout'
  try:
    serviceConfig = config.service(serviceName) if config else registry.config(serviceName)
    print(serviceConfig.to_string())
  except Exception: serviceConfig = None
  if serviceConfig and serviceConfig.has_key('SortingParameters'):
    mapperConfig = galleryUtils.getTableIfPresent(serviceConfig, 'SortingParameters')
    print(mapperConfig.to_string())
  else:
    serviceName = 'ExptGeoHelperInterface'
    serviceConfig = config.service(serviceName) if config else registry.config(serviceName)
    if serviceConfig is None:
      raise RuntimeError("Failed to retrieve the configuration for %s service" % serviceName)
    if serviceConfig.get(str)('service_provider') != 'IcarusGeometryHelper':
      raise RuntimeError(
      "{} in configuration is '{}', not IcarusGeometryHelper"
      .format(serviceName, serviceConfig['service_provider'])
      )
    # if
    mapperConfig = galleryUtils.getTableIfPresent(serviceConfig, 'Mapper')
  # if no mapper in geometry service (or no geometry service??)

  if mapperConfig:
    try:
      plugin_type = mapperConfig.get[str]('tool_type')
    except:
      raise RuntimeError(
        "{} service configuration of channel mapping is missing the tool_type:\n{}"
        .format(serviceName, mapperConfig.to_indented_string("  "))
        )
    # try ... except
  else: plugin_type = DefaultChannelMapping

  return plugin_type
# getConfiguration()

def getInitializerConfiguration(
  config: "ConfigurationClass object with complete job configuration",
  registry: "ServiceRegistryClass object with the configuration of all services",
  serviceName: "Name of the service",
  ) -> "Class object for the proper channel mapping":

  try:
    serviceConfig = config.service(serviceName) if config else registry.config(serviceName)
    print(serviceConfig.to_string())
  except Exception: serviceConfig = None
  if serviceConfig and serviceConfig.has_key('ReadoutInitializer'):
    initializerConfig = galleryUtils.getTableIfPresent(serviceConfig, 'ReadoutInitializer')
    print(initializerConfig.to_string())
  else:
    return None


  try:
    plugin_type = initializerConfig.get[str]('tool_type')
  except:
    raise RuntimeError(
      "{} service configuration of channel mapping is missing the tool_type:\n{}"
      .format(serviceName, mapperConfig.to_indented_string("  "))
      )
  # try ... except

  return plugin_type


# loadICARUSAuxDetInitializer()


def loadICARUSClass(
  config: "ConfigurationClass object with complete job configuration",
  registry: "ServiceRegistryClass object with the configuration of all services",
  serviceName: "Name of the service",
  initializer: "If true, looks for the initializer" = False,
  ) -> "Class object for the proper channel mapping":

  #
  # we need to:
  # 1. find out which mapping is required
  # 2. load the proper libraries
  # 3. return the Python class object for the mapping class we want
  #

  #
  # 1. find out which mapping is required: known configurations
  #
  if initializer:
    plugin_type = getInitializerConfiguration(config=config, registry=registry, serviceName=serviceName)
    if plugin_type is None:
      return None
  else:
    plugin_type = getConfiguration(config=config, registry=registry, serviceName=serviceName)
  print('plugin_type', plugin_type)


  #
  # 2. load the proper libraries
  #

  # get the specification record
  try: mappingInfo = ICARUSchannelMappings[plugin_type]
  except KeyError:
    # when you get to this error, check that the tool name in the configuration
    # is actually spelled correctly first...
    raise RuntimeError(
     "Mapping plug in not supported: '{}': Python library needs to be updated."
     .format(plugin_type)
     )
  # try ... except

  # load the libraries
  for codeObj in mappingInfo.get('load', []):
    LArSoftUtils.SourceCode.load(codeObj)

  # get the class object
  try: mapperClass = ROOTutils.getROOTclass(mappingInfo['mapperClassName'])
  except AttributeError:
    # this needs investigation, as the code above should be sufficient to it
    raise RuntimeError(
      "The library with '{}' has not been correctly loaded!"
      .format(mappingInfo['mapperClassName'])
      )
  # try ... except

  #
  # 3. return the Python class object for the mapping class we want
  #
  return mapperClass

# loadICARUSClass()


def loadICARUSgeometry(
  config = None, registry = None, sorterClass = None,
  ):
  """Loads and returns ICARUS geometry with the standard ICARUS channel mapping.

  See `loadGeometry()` for the meaning of the arguments.
  """
  if sorterClass is None:
    sorterClass = loadICARUSClass(config=config, registry=registry, serviceName='Geometry')

  return LArSoftUtils.loadGeometry \
    (config=config, registry=registry, sorter=sorterClass)
# loadICARUSgeometry()


def justLoadICARUSgeometry(configFile, mappingClass = None):
  """Loads and returns ICARUS geometry from the specified configuration file.

  This is a one-stop procedure recommended only when running interactively.
  """
  return loadICARUSgeometry(config=LArSoftUtils.ConfigurationClass(configFile))
# justLoadICARUSgeometry()


def loadICARUSwireReadout(
  config = None, registry = None, sorterClass = None, mappingClass = None,
  ):
  """Loads and returns ICARUS geometry with the standard ICARUS channel mapping.

  See `loadGeometry()` for the meaning of the arguments.
  """

  if sorterClass is None:
    sorterClass = loadICARUSClass(config=config, registry=registry, serviceName='Geometry')

  if mappingClass is None:
    mappingClass = loadICARUSClass(config=config, registry=registry, serviceName='WireReadout')

  return LArSoftUtils.loadWireReadout \
    (config=config, registry=registry, mapping=mappingClass, sorter=sorterClass)
# loadICARUSwireReadout()


def justLoadICARUSwireReadout(configFile, mappingClass = None):
  """Loads and returns ICARUS geometry from the specified configuration file.

  This is a one-stop procedure recommended only when running interactively.
  """
  return loadICARUSwireReadout(config=LArSoftUtils.ConfigurationClass(configFile))
# justLoadICARUSgeometry()


def loadICARUSAuxDetGeometry(
  config = None, registry = None, sorterClass = None, mappingClass = None, auxdetini = None
  ):
  """Loads and returns ICARUS geometry with the standard ICARUS channel mapping.

  See `loadGeometry()` for the meaning of the arguments.
  """

  if auxdetini is None:
    auxdetini = loadICARUSClass(config=config, registry=registry, serviceName='AuxDetGeometry', initializer=True)

  if sorterClass is None:
    sorterClass = loadICARUSClass(config=config, registry=registry, serviceName='AuxDetGeometry')

  if mappingClass is None:
    mappingClass = loadICARUSClass(config=config, registry=registry, serviceName='WireReadout')

  return LArSoftUtils.loadAuxDetGeometry \
    (config=config, registry=registry, sorter=sorterClass, auxdetinit=auxdetini)
# loadICARUSAuxDetGeometry()


def justLoadICARUSAuxDetGeometry(configFile, mappingClass = None):
  """Loads and returns ICARUS geometry from the specified configuration file.

  This is a one-stop procedure recommended only when running interactively.
  """
  return loadICARUSAuxDetGeometry(config=LArSoftUtils.ConfigurationClass(configFile))
# justLoadICAAuxDetGeometry()

################################################################################
