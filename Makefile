CC=gcc
CFLAGS=-O3
DCFLAGS=-g -Wall -DDBG
LIBSMP3=-lmp3splt
LIBSF=-lsndfile -lm
FLACLIBS=-lFLAC
EXECUTABLES=sanhwav cajwav beglop endlop spwav2d catwav xbeglop plreader spwlev wavedl wavedl_d swavedl swavedl_d pulledl pulledl_d wtxslice routim routim_d routim_dd mks00 smedl splmoftp splmofedl smedlo smedlo_d chewaud chewaud_d mymin gsine flamain smedlo_t

# sanhwav: is wav-file's header sane, does it match up to the physical size of the file?
sanhwav: sanhwav.c
	${CC} ${CFLAGS} $^ -o $@

# "cajwav" mnemonic "WAV CAJoler". This means it assumes there is a wav header and looks for some of the text entries. After that it starts to use the ftell() command to calculate the values of the other fields in the header.
# despite its name "cajole" it does not cajole a raw into a wav, use chewaud for that.
cajwav: cajwav.c
	${CC} ${CFLAGS} $^ -o $@

# concatenates all wav files presented 
catwav: catwav.c
	${CC} ${CFLAGS} $^ -o $@

beglop: beglop.c
	${CC} ${CFLAGS} $^ -o $@

# xbeglop is a tougher form, whereby the discarded data is actually saved onto the original file. It is therefore destructive.
xbeglop: xbeglop.c
	${CC} ${CFLAGS} $^ -o $@

endlop: endlop.c
	${CC} ${CFLAGS} $^ -o $@

# split wavs to a tmpdir according to chunks sized in mm:ss.huns format.
spwav2d: spwav2d.c
	${CC} ${CFLAGS} $^ -o $@

# This one started from the spwav chunk splitter, but I incorporated options
# so it started to do more. Principally it was a 32bit sample to 16 bit sample reducer.
# under the horrible name of spmwav2d. I then changed this name to chewaud. Why not chewav,
# well it will probably do flac's as well soon.
chewaud: chewaud.c
	${CC} ${CFLAGS} $^ -o $@
chewaud_d: chewaud.c
	${CC} ${DCFLAGS} $^ -o $@

# split wavs according to a level
spwlev: spwlev.c
	${CC} ${CFLAGS} $^ -o $@

# split wavs according to a mplayer's EDL file
wavedl: wavedl.c
	${CC} ${CFLAGS} $^ -o $@

# debug version of the above
wavedl_d: wavedl.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

pulledl: pulledl.c
	${CC} ${CFLAGS} $^ -o $@
pulledl_d: pulledl.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

# a variation of the edl reading: splits (therefore the "s") entire wav up: edlpts used as single markers, not start and endpoints.
# NOTE: mplayer will only allow start- and end- edlpoint pairs. If you type "i" an uneven number of times
# the final "i" wil not be recorded.
swavedl: swavedl.c
	${CC} ${CFLAGS} $^ -o $@
swavedl_d: swavedl.c
	${CC} ${CFLAGS} -DDBG $^ -o $@
swavedl_dd: swavedl.c
	${CC} ${CFLAGS} -DDBG2 $^ -o $@

# transfers endslice of first wav to the beginning of the second
wtxslice: wtxslice.c
	${CC} ${CFLAGS} $^ -o $@

# read a playlist: actually just slurps in a file listing as an array of lines treated as strings
plreader: plreader.c
	${CC} ${CFLAGS} $^ -o $@

# routime: rough time getting time poitns out of rough text
routim: routim.c
	${CC} ${CFLAGS} -o $@ $^
routim_d: routim.c
	${CC} ${CFLAGS} -DDBG -o $@ $^
routim_dd: routim.c
	${CC} ${CFLAGS} -DDBG -DDBG2 -o $@ $^

# just read the frigging edl's
# don't forget mplayer will not use edl's if it's not video
redl: redl.c
	${CC} ${CFLAGS} -o $@ $^

# OK, this is the program that uses libmp3splt and an edl file.
# However, the conversion of the timings are not great.
# # note this wil also split oggs, but outer and inner splits also taken .. only in ogg not in mp3, weird huh?
smedl: smedl.c
	${CC} ${CFLAGS} -o $@ $^ ${LIBSMP3}
smedl_d: smedl.c
	${CC} ${DCFLAGS} -o $@ $^ ${LIBSMP3}
# unfort, the issue of the offset required is pretty tricky ...allow it as an argument
smedlo: smedlo.c
	${CC} ${CFLAGS} -DEDL -o $@ $^ ${LIBSMP3}
smedlo_d: smedlo.c
	${CC} ${DCFLAGS} -DEDL -o $@ $^ ${LIBSMP3}
smedlo_t: smedlo.c
	${CC} ${DCFLAGS} -DTNUM -o $@ $^ ${LIBSMP3}

# Mnemonic for this is SPLit Mp3 or Ogg Full Time Point
# it will take a series of 0:0.0 points on the time line split the mp3/ogg in full (i.e. segments on either side of splitpoints) 
# In this way the original mp3/ogg can be deleted.
splmoftp: splmoftp.c
	${CC} ${DCFLAGS} -o $@ $^ ${LIBSMP3}
splmofedl: splmofedl.c
	${CC} ${DCFLAGS} -o $@ $^ ${LIBSMP3}

mymin: mymin.c
	${CC} ${CFLAGS} -o $@ $^ ${LIBSMP3}

# some simulation programs required.

mks00: mks00.c
	${CC} ${DCFLAGS} -o $@ $^ ${LIBSF}

gsine: gsine.c
	${CC} ${DCFLAGS} -o $@ $^ -lm

flamain: flamain.c
	${CC} ${DCFLAGS} -o $@ $^ $(FLACLIBS)

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
