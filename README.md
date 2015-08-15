# "wavchudit" repository
A set of command-line tools to edit wav files. Coded in C with only C standard library dependencies. Useful for breaking wavfiles into smaller, more manageable chunks. Some wav files can be very big, so it's besti not to read them into memory in one go, and rather navigate them as files using fseek. However, if there is no effort in this code to limit the size of the wav chuns that are read in, and sometimes these can be big 8>2G) too.

In essence, these tools are very similar to certain small functions of the SoX program, but with a different slant.

Why wav? Sure, that's a good question to ask, they tend to be big files. Well, they also can be edited losslessly, and are often
an intermediate audio format. In any case, editing audio can be quite fiddly, and you will prbably only edit one wavfile that's big at a time.

# introduction
Many audio editting tools are presented via a GUI, where the waveform can be seen
and the file "navigated". Examples are cooledit, goldwave, audacity, and they are quite user friendly. However, they
are also somewhat rough (opening and saving a wav file without any editing whatsoever, will not give you the exact same
file: somewhat suspicious). Also, a GUI means that bulk processing may take a backrow seat.

So I came up with these tools in order to have greater control, and introduce the idea of "blind" wav-file editing. You will need to use
sox or mplayer to establish the timings of interest, so you can decide where to edit. Mplayer is the best auditory navigator, enabling you to jump in three sizes of steps: by defualt these are 10 secs, 60 secs or 600 secs and tese can be varied. It's timing is not entirely perfect, but getting the timing of certain edit point exactly right is not easy, whatever medium you choose. Often, you decide that you want another edit point at the last minute, so it's question of living with this inexactitude.

# modus operandi
You usually are using sox's play or mplayer to listen to a wav-file, and decide you are only interested in a certain part. As is the case with "blind" editing, you only have a basic idea of the timing. Often, you have not written it down, it's only in your head. So these are things you can do
* decide you are not interested in the rest of the file. use endlop from the time of the last interesting bit to get rid of the end.
* decide that the beginning of the file is not interesting.
(Note how you only need one, not the more common two - timings for these two operations. Commonly, you will use one and then the other
perhaps several times to close in on the area of interest.

# WAV/RIFF data format notes
Current dependencies are only the standard C library. I did start by using libsndfile, but in truth, the wav data structure is rather simple, so I made the code independent of libsndfile, so now it's self-standing, so to speak.

## The size limitation of the "int" type
The first thing to note is that 4 bytes (ints) are used to hold the length of the data. Ths means that very big wavs (over 2G and a bit or so) will probably overflow the type. You're better off not depending on this field (BYtes In Data: byid) and calculating hte size of the file in the code and storing it as 8 bytes long or unsigned long.

## the so-called BYtes Per Cpature header field
the BYtes Per Capture field is a little awry, and it seems a mistake to depend upon it. For the canonical 16bit sampling it should be 2, but in many stereo wavs, it reads 4. WAVS with bypc=2 and bypc=4 get treated the same way and "soxi" at least reports 16bit always. This could be a problem if 24bit sampling is used, which would need ints instead of shorts. In any case, the way around this inconsistency is to use BItsPerSAMPleS divided by 8, to work out whether "short"s or "int"s should be used.

## How to think and work in sample
It's easy to get a little confused with the naming conventions implied by the WAV/RIFF header. Overall however, it's best to think in samples. Whether shorts are used to quantise their values, or whether they are single or 2 channel are part of the nature of "a sample", and will need to be decided at the beginning, when reading from a wav, and at the end, when writing to one.

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
* wavedl, needs to be run in conjunction with mplayer's -edlout switch. When navigating a wav file with mplayer, this mode records time points whne a first "i" is typed, and a second "i" is typed and stores them in a text file. this program reads the text file and outputs a wavfile for each edl first-second pair.
* pulledl: please run without arguments. Requires an mplayer generated EDL file.
* wtxslice: takes two wavfiles and transfer an endslice of the first to the beginning of the second. Slice point - the third argument, no prizes for guessing which the first and second arguments are - must be within the length of the first wav file and is in mins:secs.centisecs format.
