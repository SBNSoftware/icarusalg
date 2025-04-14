#!/usr/bin/env python

__doc__ = """
Provides a service manager preconfigured with ICARUS service providers.

A `ServiceManager` object bridges the access to the service providers.
In the most straightforward cases, getting a service provider is as simple as
calling `ServiceManager(providerName)` with the name of the service as string
(e.g. `larProp = ServiceManager('LArProperties')`).



If this module is executed as a script, the service manager is loaded and a
python interactive session is started. The main code also show how to override
the service manager setup by choosing a different service configuration.
"""

__all__ = [ 'ServiceManager', 'geometry', 'wireReadout' ]


import ICARUSutils  # loadICARUSgeometry(), ...
import LArSoftUtils
from cppUtils import UnusedAttr


################################################################################
### special known services
###

class ICARUSGeometryServiceGetter(LArSoftUtils.GeometryServiceGetter):
  
  def _loadService(self, manager, dependencies: UnusedAttr = {}):
    return ICARUSutils.loadICARUSgeometry(registry=manager.registry())
  
# class ICARUSGeometryServiceGetter

class ICARUSWireReadoutServiceGetter(LArSoftUtils.WireReadoutServiceGetter):
  
  def _loadService(self, manager, dependencies = dict(Geometry=None)):
    return ICARUSutils.loadICARUSwireReadout \
      (registry=manager.registry(), geometry=dependencies['Geometry'])
  
# class ICARUSWireReadoutServiceGetter

class ICARUSAuxDetGeometryServiceGetter(LArSoftUtils.AuxDetGeometryServiceGetter):
  
  def _loadService(self, manager, dependencies: UnusedAttr = {}):
    return ICARUSutils.loadICARUSauxDetgeometry(registry=manager.registry())
  
# class ICARUSAuxDetGeometryServiceGetter


################################################################################
###  module setup - part I
###
###  Where global service manager is set up.
###

class ICARUSserviceManagerClass(LArSoftUtils.ServiceManagerInstance):

  DefaultConfigPath = "services_basic_icarus.fcl"
  DefaultServiceTable = "icarus_basic_services"

  def defaultConfiguration(self):
    """

    Configuration:

    If `serviceTable` is not `None`, a new configuration is created with the
    service table as `serviceTable`, and `configPath` is included in that
    configuration (presumably to define `serviceTable`). In this case, if
    `configPath` is `None`, "services_icarus_simulation.fcl" is included.

    If both `configPath` and `serviceTable` are `None`, the configuration is
    created as above, using "icarus_simulation_services" as `serviceTable`.

    Finally, if only `serviceTable` is `None`, the configuration file in
    `configPath` is included directly, and it is assumed that it already
    properly defines a `services` table.
    """
    return ICARUSserviceManagerClass.DefaultConfigPath, ICARUSserviceManagerClass.DefaultServiceTable
  # defaultConfiguration()

  def __init__(self):
    super().__init__()
    self.setConfiguration(
      configFile=ICARUSserviceManagerClass.DefaultConfigPath,
      serviceTable=ICARUSserviceManagerClass.DefaultServiceTable,
      )
  # __init__()

  def setup(self):
    """Prepares for ICARUS service provider access in python/Gallery."""

    super().setup()

    #
    # register the services we know about;
    # some are already known
    # (`LArSoftUtils.ServiceManagerClass.StandardLoadingTable`), including
    # 'Geometry', 'LArProperties', 'DetectorClocks' and 'DetectorProperties',
    # but we override the former with our flavor of it
    #

    self.manager.registerLoader('Geometry', ICARUSGeometryServiceGetter())
    self.manager.registerLoader('WireReadout', ICARUSWireReadoutServiceGetter())
    self.manager.registerLoader('AuxDetGeometry', ICARUSAuxDetGeometryServiceGetter())
    
    return self.manager

  # setup()

# class ICARUSserviceManagerClass


ServiceManager = ICARUSserviceManagerClass()


################################################################################

def geometry():    return ServiceManager.get('Geometry')
def wireReadout(): return ServiceManager.get('WireReadout')


################################################################################

if __name__ == "__main__":

  #
  # unfortunately, ROOT module interferes with command line arguments.
  #
  import argparse

  Parser = argparse.ArgumentParser(
    description=
      "Starts a python interactive session with `ServiceManager` available."
    )

  Parser.add_argument("--config", "-c", dest="configPath",
    help="configuration file path (must define `services` or host serviceTable below)"
    )
  Parser.add_argument("--servicetable", "-T", dest="serviceTable",
    help="name of the FHiCL table where all services are configured")

  args = Parser.parse_args()

  if args.configPath is not None:
    ServiceManager.setConfiguration(args.configPath, args.serviceTable)

  # we want ROOT module known in the interactive session;
  # and we keep the good habit of loading it via ROOTutils
  from ROOTutils import ROOT

  try:
    import IPython
    IPython.embed()
  except ImportError:
    import code
    code.interact(local=dict(globals(), **locals()))
  # try ... except

# main
