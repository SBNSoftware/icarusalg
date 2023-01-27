#!/usr/bin/env python

import sys, re, logging

__doc__  = """Parses a art log from DumpPMTconfiguration module and returns the settings in some format."""
__author__ = "Gianluca Petrillo (petrillo@slac.stanford.edu)"
__date__   = "February 3, 2023"


def extractPMTreadoutSettings(inputFile: "an already opened log file") -> "a sorted list of (channel, board, hwchannel, baseline, threshold)":
  
  logger = logging.getLogger() # may change to a specific instance
  
  ChannelPattern = re.compile(r'board channel #(\d+) \(.*\), offline ID: (\d+), baseline: (\d+), threshold: (\d+) \(delta=\d+\)')
  class ChannelPatternIndices:
    BoardChannel = 1
    ChannelID = 2
    HWBaseline = 3
    HWThreshold = 4
  # class ChannelPatternIndices
  
  BoardPattern = re.compile(r"\[B\d+\]: board name: '(.*)' \(ID: (\d+); fragment ID: (\d+)\), (\d+) configured channels")
  class BoardPatternIndices:
    Name = 1
    Number = 2
    FragmentID = 3
    NChannels = 4

  Settings = []
  currentBoard = None
  channelsInBoard = set()
  expectedChannels = None
  for iLine, line in enumerate(inputFile.readlines()):
    line = line.strip()
    
    if (match := ChannelPattern.search(line)):
      assert currentBoard is not None, f"At line {iLine}: missed the board number in the input log file."
      boardChannel = int(match.group(ChannelPatternIndices.BoardChannel))
      if boardChannel in channelsInBoard:
        raise RuntimeError("At line {iLine}: missed a board number in the input log file, board channel {boardChannel} was already found in board {currentBoard}.")
      Settings.append(dict(
        Channel=int(match.group(ChannelPatternIndices.ChannelID)),
        BoardNumber=currentBoard,
        BoardChannel=boardChannel,
        Baseline=int(match.group(ChannelPatternIndices.HWBaseline)),
        Threshold=int(match.group(ChannelPatternIndices.HWThreshold)),
        ))
      channelsInBoard.add(boardChannel)
    elif (match := BoardPattern.search(line)):
      if expectedChannels is not None and len(channelsInBoard) != expectedChannels:
        logger.warning(f"Board {currentBoard} has only {len(channelsInBoard)} channels ({expectedChannels} were expected): {', '.join(map(str, channelsInBoard))}")
      channelsInBoard = set()
      currentBoard = int(match.group(BoardPatternIndices.Number))
      expectedChannels = int(match.group(BoardPatternIndices.NChannels))
    else:
      continue
    
  # for
  
  Settings.sort(key=lambda d: d['Channel'])
  return Settings
# extractPMTreadoutSettings()


def entryToFHiCL(entry, params = {}):
  paddings = params.get('padding', {})
  keys = params.get('keys', entry.keys())
  return (
      "{ "
    + ", ".join(f"{key}: {entry[key]:{paddings.get(key, 0)}d}" for key in keys)
    + " }"
    )
  
# entryToFHiCL()

def entryToPython(entry, params = {}):
  paddings = params.get('padding', {})
  keys = params.get('keys', entry.keys())
  return (
      "{ "
    + "".join(f"'{key}': {entry[key]:{paddings.get(key, 0)}d}, " for key in keys)
    + "}"
    )
  
# entryToPython()

def extractValuePaddings(entries):
  paddings = {}
  for entry in entries:
    for k, v in entry.items():
      paddings[k] = max(paddings.get(k, 0), len(str(v)))
  return paddings
# def extractValuePaddings()


def formatData(settings, args):
  Paddings = extractValuePaddings(settings)
  Keys = ( 'Channel', 'BoardNumber', 'BoardChannel', 'Baseline', 'Threshold' )
  params = dict(padding=Paddings, keys=Keys)
  if args.outputFormat == "FHiCL":
    return (
      ((args.varName + " : ") if args.varName else "") + "[\n"
      + ",\n".join(("  " + entryToFHiCL(entry, params)) for entry in settings)
      + f"] {('# ' + args.varName) if args.varName else ''}\n"
      )
  elif args.outputFormat == "Python":
    return (
      ((args.varName + " = ") if args.varName else "") + "["
      + "".join(("\n  " + entryToPython(entry, params) + ",") for entry in settings)
      + f"\n] {('# ' + args.varName) if args.varName else ''}\n"
      )
  else:
    raise NotImplementedError(f"Output format '{args.outputFormat}' not implemented!")
# formatEntry()


if __name__ == "__main__":
  
  logging.basicConfig()
  
  import argparse
  
  parser = argparse.ArgumentParser(description='Process some integers.')
  parser.add_argument('inputLog', help='the log to be parsed')
  parser.add_argument('--format', dest='outputFormat',
    choices=[ 'FHiCL', 'Python' ], default='Python',
    help='output format [%(default)s]',
    )
  parser.add_argument('--varname', dest='varName', default=None,
    help='the output data structure is assigned to a variable with this name',
    )
  
  args = parser.parse_args()
  
  Settings = extractPMTreadoutSettings(open(args.inputLog) if args.inputLog else sys.stdin)
  
  # this is the format required by `DrawWaveforms` utility:
  print(formatData(Settings, args))
  
  sys.exit(0)
# main
