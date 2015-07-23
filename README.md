# wavchudit
A set of tools to edit wav files

## dependencies
only the standard C library. I did start by using libsndfile,
but in truth, the wav data structure is rather easy, so I made
the code independent of libsndfile, so no wit's self-standing

# initial scope
Merely to extract chunks out of wav files. Plenty of tools do this to other sound formats,
and I'm sure other tools actually do the same as this

## character
the tool is to be enitrely command-line. No GUI.

## associations
this is not a player of wav-files, nor does it enable timings. I recomend sox or mplayer for these tasks.
Before you edit wav-files you definitely will have needed to analyse them with this tools.

# components
endlop: lop off the end of the wav file. will output the beginning part of a wav file. Original file
left untouched
beglop
