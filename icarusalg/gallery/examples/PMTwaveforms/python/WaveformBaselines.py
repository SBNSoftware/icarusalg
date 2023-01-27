#!/usr/bin/env python

import galleryUtils
import ROOT
import numpy

__doc__  = """Simple waveform baseline computation."""
__author__ = "Gianluca Petrillo (petrillo@slac.stanford.edu)"
__date__   = "February 3, 2023"


def groupPMTsByChannel(Waveforms, NChannels = None):
  WaveformsByChannelDict = {}
  for wave in Waveforms:
    WaveformsByChannelDict.setdefault(wave.ChannelNumber(), []).append(wave)
  if not NChannels: NChannels = max(WaveformsByChannelDict) + 1
  return [ WaveformsByChannelDict.get(ch, []) for ch in range(NChannels) ]
# groupPMTsByChannel()


def computeBaseline(waveforms: "list of waveforms sharing a baseline") -> "baseline estimation":
  """Crude baseline estimation: median from all the samples of the waveform."""
  samples = numpy.concatenate(channelWaveforms)
  return numpy.median(samples)
# computeBaseline()


if __name__ == "__main__":
  
  import argparse
  
  parser = argparse.ArgumentParser(description=__doc__)

  parser.add_argument('inputFiles', nargs='+',
    help='path of input file(s) or file lists')
  parser.add_argument('-n', '--nevts', dest='MaxEvents', type=int, default=None,
    help='maximum number of events to process [all]')
  parser.add_argument('--nskip', dest='NSkip', type=int, default=0,
    help='skip this number of events from the first one [none]')
  parser.add_argument('--tag', dest='OpDetTag', default="daqPMT",
    help='tag of the waveform data product [%(default)s]')

  args = parser.parse_args()
  
  sampleEvents = galleryUtils.makeEvent(*args.inputFiles)
  OpDetTag = ROOT.art.InputTag(args.OpDetTag)
  
  nEvents = 0
  getOpDetWaveformHandle = galleryUtils.make_getValidHandle("std::vector<raw::OpDetWaveform>", sampleEvents)
  for iEvent, event in enumerate(galleryUtils.forEach(sampleEvents)):
    if iEvent < args.NSkip: continue
    if args.MaxEvents is not None and nEvents >= args.MaxEvents:
      print(f"Maximum number of events reached ({args.MaxEvents}).")
      break
    nEvents += 1
    
    Waveforms = getOpDetWaveformHandle(OpDetTag).product()
    WaveformsByChannel = groupPMTsByChannel(Waveforms)
    print(f"{event.eventAuxiliary().id()}: {len(Waveforms)} waveforms on {len(WaveformsByChannel)} channels")
    
    for channel, channelWaveforms in enumerate(WaveformsByChannel):
      baseline = computeBaseline(channelWaveforms)
      print(f"Channel {channel:3d}: baseline {baseline:g} from {len(channelWaveforms)} PMT waveforms")
    # for channels
  # for all events
  
# main


