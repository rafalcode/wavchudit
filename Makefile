CC=gcc
CFLAGS=-g -Wall

EXECUTABLES=sanhwav cajwav beglop endlop spwav2d catwav xbeglop plreader spwlev wavedl wavedl_d swavedl swavedl_d pulledl pulledl_d wtxslice routim routim_d routim_dd

# sanhwav: is wav-file's header sane, does it match up to the physical size of the file?
sanhwav: sanhwav.c
	${CC} ${CFLAGS} $^ -o $@

# "cajwav" mnemonic "WAV CAJoler". This means it assumes there is a wav header and looks for some of the text entries. After that it starts to use the ftell() command to calculate the values of the other fields in the header.
cajwav: cajwav.c
	${CC} ${CFLAGS} $^ -o $@

# concatenates all wav files presented 
catwav: catwav.c
	${CC} ${CFLAGS} $^ -o $@

beglop: beglop.c
	${CC} ${CFLAGS} $^ -o $@

# xbeglop is a tougher form, whereby the dicarded data is actually saved onto the original file. It is therefore destructive.
xbeglop: xbeglop.c
	${CC} ${CFLAGS} $^ -o $@

endlop: endlop.c
	${CC} ${CFLAGS} $^ -o $@

# split wavs to a tmpdir according to chunks sized in mm:ss.huns format.
spwav2d: spwav2d.c
	${CC} ${CFLAGS} $^ -o $@

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

# transfers endslice of first wav to the beginning of the second
wtxslice: wtxslice.c
	${CC} ${CFLAGS} $^ -o $@

# read a playlist: actually just slurps in a file listing as an array of lines treated as strings
plreader: plreader.c
	${CC} ${CFLAGS} $^ -o $@

# routime: rough time
routim: routim.c
	${CC} ${CFLAGS} -o $@ $^
routim_d: routim.c
	${CC} ${CFLAGS} -DDBG -o $@ $^
routim_dd: routim.c
	${CC} ${CFLAGS} -DDBG -DDBG2 -o $@ $^

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
