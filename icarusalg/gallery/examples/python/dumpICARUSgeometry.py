#!/usr/bin/env python

__doc__    = """Dumps the ICARUS geometry on screen."""
__author__ = "Gianluca Petrillo (petrillo@slac.stanford.edu)"
__date__   = "Tue Mar 18 2025"

from ICARUSservices import ServiceManager
from cppUtils import SourceCode
from ROOTutils import ROOT

if __name__ == '__main__':
  # using the "standard" configuration
  
  import argparse, sys
  
  Parser = argparse.ArgumentParser(description=__doc__)

  Parser.add_argument("--config", "-c", dest="configPath",
    help="configuration file path (must define `services` or host serviceTable below)"
    )
  Parser.add_argument("--servicetable", "-T", dest="serviceTable",
    help="name of the FHiCL table where all services are configured")
  Parser.add_argument("--output", "-o",
    help="write the output into this file [stdout]")
  Parser.add_argument("--append", "-a", action='store_true',
    help="if writing to a file, add to its end [default: remove old content]")

  args = Parser.parse_args()

  if args.configPath or args.serviceTable:
    ServiceManager.setConfiguration(args.configPath, args.serviceTable)

  # load the necessary services
  geom = ServiceManager.get('Geometry')
  wireReadout = ServiceManager.get('WireReadout')
  auxDetGeom = ServiceManager.get('AuxDetGeometry')

  # load the dumping algorithm
  SourceCode.loadHeaderFromUPS("larcorealg/Geometry/WireReadoutDumper.h")
  SourceCode.loadLibrary("larcorealg_Geometry")
  dumper = ROOT.geo.WireReadoutDumper(geom, wireReadout, auxDetGeom)

  # prepare the output file and run the dump
  if args.output:
    outputFile = ROOT.std.ofstream(args.output, ROOT.std.ios.app if args.append else ROOT.std.ios.trunc)
    print(f"Writing the output to '{args.output}'.")
  else:
    outputFile = ROOT.std.cout
  dumper.dump(outputFile)

  # Only losers close their files.
  sys.exit(0)
# __main__
