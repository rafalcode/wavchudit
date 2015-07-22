CC=gcc
CFLAGS=-g -Wall
BINLOC=${HOME}/mylocal/bin

EXECUTABLES=sanwav cajwav beglop endlop spwav2d catwav xbeglop

# sanwav is wav-file sanity check program. It checks the files on the commandline and printsout key header values. Generally, one hopes for 44100sampfq, 1 short per capture.
sanwav: sanwav.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

# "cajwav" mnemonic "WAV CAJoler". This means it assumes there is a wav header and looks for some of the text entries. After that it starts to use the ftell() command to calculate the values of the other fields in the header.
cajwav: cajwav.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

# concatenates all wav files presented 
catwav: catwav.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

beglop: beglop.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

# xbeglop is a tougher form, whereby the dicarded data is actually saved onto the original file. It is therefore destructive.
xbeglop: xbeglop.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

endlop: endlop.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

# split wavs to a tmpdir according to chunks sized in mm:ss.huns format.
spwav2d: spwav2d.c
	${CC} ${CFLAGS} $^ -o $@
	cp $@ $(BINLOC)

.PHONY: clean

clean:
	rm -f ${EXECUTABLES}
