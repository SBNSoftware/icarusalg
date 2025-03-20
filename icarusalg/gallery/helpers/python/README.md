# ICARUS utilities for interface with Python {#ICARUS_Python_README}

| Topic:          | ICARUS/LArSoft code via Python                 |
| --------------- | ---------------------------------------------- |
| Type:           | Python general documentation                   |
| Author:         | Gianluca Petrillo (petrillo@slac.stanford.edu) |
| Created on:     | June 23, 2019                                  |
| Version:        | 1.2                                            |


This document describes the utilities provided in `icaruscode` to facilitate the
use of LArSoft and ICARUS-specific code developed for _art_/_gallery_ into
Python.

This kind of documents tend to become outdated very quickly, so the reader is
invited, when in doubt, to refer to the code itself. Most of the utilities that
are designed to be part of the interface to users have some terse documentation
in the form of python documentation strings.


Format of this document
------------------------

This file (`README.md`) is written in a dialect of [Markdown] format that is
compatible with [`pandoc`][pandoc] and [Doxygen]. The format is designed to be
readable as plain text: in that case, the reader will find all the links to
external resources at the end of the document.
The program `pandoc` allows to render this file in other formats. For example,
to render it into Portable Document Format:
    
    pandoc --toc -o README.pdf README.md
    
and to render it into HyperText Transfer Protocol format:
    
    pandoc --toc -o README.html README.md
    


Introduction to the interface to Python
========================================

The utilities provided here help the configuration of LArSoft service providers
and the access to data products in a Python environment without _art_, but with
canvas/gallery.

The more low level utilities are installed from `sbnalg`, while in `icarusalg`
only the ICARUS-specific customizations are present.
In particular, the functions to correctly load some essential services like the
geometry service set.
This interface is compatible with LArSoft v10 and newer.

Components of the Python support libraries
===========================================

In addition to the lower level utilities provided by `sbnalg` and documented
in their own `README.md`, `icarusalg` offers:

* `ICARUSutils.py` includes functions to load ICARUS-specific service providers
  (like ICARUS geometry implementation)
* `ICARUSservices.py` facilitates the management of service providers in the
  context of an ICARUS execution environment, with fitting defaults.
  It can also used as an executable to start an interactive section where some
  initialization has already automatically taken place.

In the following sections, details are provided on the utilities from these
libraries that are expected to be most commonly used.
Some of the newest facilities are documented inline in Python. In that case,
help can be obtained interactively (for example, the command to access the
documentation of the object `ICARUSservices.ServiceManager` is
`help(ICARUSservices.ServiceManager)`).


`ICARUSservices.py`: service providers (and more) in ICARUS context
--------------------------------------------------------------------

`ICARUSservices.py` may be used as library in a Python script, or directly to
launch an interactive session.
It provides easier access to LArSoft service providers in Python.


### Interactive Python session configured for ICARUS

The interactive section can be started by just executing
    
    python ICARUSservices.py
    
Curious to see something about the geometry of the wire planes? or detector
timing settings? the standard methods of service providers can be easily
accessed:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.python}
wireReadout = ServiceManager('WireReadout')
for plane in wireReadout.Iterate[ROOT.geo.PlaneGeo](): print(plane.PlaneInfo())

detClocks = ServiceManager('DetectorClocks')
detClocks.debugReport()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
(funny choice of example: currently (icaruscode v10.4.4) cppyy is not able to
cope with `wireReadout.Iterate[ROOT.geo.PlaneGeo]()`)

Supported service providers include: `Geometry`, `WireReadout`,
`AuxDetGeometry`, `DetectorClocks`, `LArProperties`, `DetectorProperties`.


### Library utilities

`ServiceManager`
  
:   object to load and keep track of services.
    It is configured by FHiCL and its configuration must include all the
    services that are going to be requested, in a way similar to a regular
    LArSoft job configuration (e.g. in a `services` table).
    
    `ServiceManager.setConfiguration(configPath, serviceTable)` selects the
    configuration file (`configPath`) and the FHiCL table (`"services"` by
    default) where the configuration of the services is stored; all the
    following requests for new services will use this configuration.
    The default configuration is defined in `ICARUSserviceManagerClass` and in
    `v10_04_04` is the `icarus_basic_services` configuration table taken from
    `services_basic_icarus.fcl`.
    
    An example of using `ServiceManager`:
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.python}
    from ICARUSservices import ServiceManager
    ServiceManager.setConfiguration \
      ('services_basic_icarus.fcl', 'icarus_basic_services')
    detClocks = ServiceManager('DetectorClocks')
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    (this is actually the default configuration in `v10_04_04`) uses an
    essential set of standard service configuration tables of ICARUS,
    `icarus_simulation_services`, from `services_icarus_simulation.fcl`
    configuration file. That file is found in the usual way via
    `FHICL_FILE_PATH`, which should have been set already during the environment 
    setup.
    
    > _Note_: The `ServiceManager` object interface is defined in
    >         `LArSoftUtils.py` (`sbnalg`). As a consequence, the code using it
    >         may often be transparently ported or shared between ICARUS and
    >         SBND.

`geometry()`

:   shortcut for `ServiceManager('Geometry')`: loads and returns an instance of
    LArSoft geometry service provider (`geo::GeometryCore`) with ICARUS
    configuration as found in the `Geometry` entry of the service configuration
    table described above.

`wireReadout()`

:   shortcut for `ServiceManager('WireReadout')`: loads and returns an instance
    of LArSoft wire readout geometry service provider (`geo::WireReadoutGeom`)
    with configuration as found in the `WireReadout` entry of the service
    configuration table described above.


`ICARUSutils.py`: lower level ICARUS-specific functions
--------------------------------------------------------

`ICARUSutils.py` provides ICARUS-specific functions:

* geometry service providers for ICARUS:

    `loadICARUSgeometry`, `loadICARUSwireReadout`, `loadICARUSauxDetGeometry`

    :   loads and returns ICARUS geometry/wire readout/auxiliary detector
        geometry service provider with the standard configuration. It is usually
        easier to go through `ServiceManager` in `ICARUSservices.py`,
        which uses this facility anyway.


Execution environment
======================

This facility includes interactions between many diverse components: _gallery_,
ROOT, LArSoft, ICARUS code, Python... While it has been proven that this _can_
work, the setup of a working environment might be non-trivial.
The following recipes has proven to work on ICARUS SL7 GPVM servers:

* use a binary distribution, setting it up with `setup icaruscode ...`
* use a MRB area with all of the following:
    * `icaruscode` checked out in the source directory, or explicitly set up;
      e.g. `( cd "$SOURCE_DIR" && mrb gitCheckout icaruscode ; )`
    * all checked out code properly compiled _and installed_: `mrb install -j4`
    * all local "products" set up: `mrbslp`

Both setup procedures should set up all is needed, including important parts
like:

* UPS environment variables, like `LARCOREALG_INC`, used to locate header files
  and compiled libraries
* ROOT machinery (`ROOTSYS` and more)
* `PYTHONPATH` environment variable, used to locate the Python modules described
  in this document

In addition, to use _gallery_, it must be explicitly set up. The easiest way to
get there is to set up `larsoftobj`:

* check which `larsoftobj` version is matching the LArSoft version in MRB
  working area in the [LArSoft release web page][LArRel]
* after the setup procedure described above, set up `larsoftobj` as well


Examples
=========

Additional examples are available in the documentation of the lower level Python
libraries in `sbnalg`.


Geometry dump
--------------

ICARUS geometry can be dumped on screen by calling a dumping algorithm in
`larcorealg`.
A full example is installed as executable script in `icarusalg` and is available
in `icarusalg/gallery/examples/python/dumpICARUSgeometry.py`.

The essence of the example is the following:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.python}
from ICARUSservices import ServiceManager
from cppUtils import SourceCode
from ROOTutils import ROOT

geom = ServiceManager.get('Geometry')
wireReadout = ServiceManager.get('WireReadout')
auxDetGeom = ServiceManager.get('AuxDetGeometry')

SourceCode.loadHeaderFromUPS("larcorealg/Geometry/WireReadoutDumper.h")
SourceCode.loadLibrary("larcorealg_Geometry")
dumper = ROOT.geo.WireReadoutDumper(geom, wireReadout, auxDetGeom)

dumper.dump(ROOT.std.cout)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here, `geo::WireReadoutDumper` is the dumping algorithm class from `larcorealg`,
which requires access to some geometry-related service providers.
The `ServiceManager` (configured with ICARUS defaults) is used to access those
service providers. `SourceCode` is used to load headers and libraries for the
algorithm (for data products which have dictionaries around, ROOT can usually
figure out on its own).

Also note that, apart from the service manager object, the example works just
the same for SBND.


[Markdown]: https://daringfireball.net/projects/markdown
[Doxygen]: http://www.doxygen.nl
[pandoc]: https://pandoc.org/MANUAL.html
