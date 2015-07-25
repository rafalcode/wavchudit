# "wavchudit" repository
A set of command-line tools to edit wav files.

# introduction
Many audio editting tools are presented via a GUI, where the waveform can be seen
and the file "navigated". Examples are cooledit, goldwave, audacity, and they are quite user friendly. However, they
are also somewhat rough (opening and saving a wav file without any editing whatsoever, will not give you the exact same
file: somewhat suspicious). Also, a GUI means that bulk processing may take a backrow seat.

So I came up with these tools in order to have greater control, and introduce the idea of "blind" wav-file editing. You will need to use
sox or mplayer to establish the timings of interest, so you can decide where to edit.

# modus operandi
You usually are using sox's play or mplayer to listen to a wav-file, and decide you are only interested in a certain part. As is the case with "blind" editing, you only have a basic idea of the timing. Often, you have not written it down, it's only in your head. So these are things you can do
* decide you are not interested in the rest of the file. use endlop from the time of the last interesting bit to get rid of the end.
* decide that the beginning of the file is not interesting.
(Note how you only need one, not the more common two - timings for these two operations. Commonly, you will use one and then the other
perhaps several times to close in on the area of interest.

## dependencies
only the standard C library. I did start by using libsndfile,
but in truth, the wav data structure is rather easy, so I made
the code independent of libsndfile, so no wit's self-standing

# initial scope
Merely to extract chunks out of wav files. Plenty of tools do this to other sound formats,
and I'm sure other tools actually do the same as this

## associations
this is not a player of wav-files, nor does it enable timings. I recomend sox or mplayer for these tasks.
Before you edit wav-files you definitely will have needed to analyse them with this tools.

# components
* sanhwav: sanity check of wav-file's header only. To see if the header is well-formed.
* beglop: lop off the beginning of a wav file. will output the end part of file. Original file left intact.
* endlop: lop off the end of a wav file. will output the beginning part of file. Original file left intact.
* xbeglop: dangerous edit, same as beglop, except it overwrites the lop off the beginning of a wav file. will output the end part of file. Original file left intact.
* spwav2d, this SPlits a WAVfile TO a DIRECTORY. Aimed for large wav-files, it allows you to specify a chunk in units of
time and it will split the wavfile into chunks of that size. If - as is probably the case, the chunk size does not divide evenly into the total wavfile size, one of the chunks will have a size less than the normal chunks size of the others.
* catwav, this is an interesting program to try out on spwav2d, because it concatenates wavfiles, as its name suggests.
