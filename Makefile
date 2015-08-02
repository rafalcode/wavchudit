CC=gcc
CFLAGS=-g -Wall

EXECUTABLES=sanhwav cajwav beglop endlop spwav2d catwav xbeglop plreader spwlev wavedl

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

wavedl_d: wavedl.c
	${CC} ${CFLAGS} -DDBG $^ -o $@

# read a playlist: actually just slurps in a file listing as an array of lines treated as strings
plreader: plreader.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
