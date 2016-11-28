
wavchudit is a simplified command-line program for editing audio files of the RIFF or wav format.
Its design has aimed at high quality editing on the linux command-line
with minimal compilation and running dependencies.

Therefore it only uses C89 std C and only uses the C library.

It also follows a certain philosophy to the editting of files which tries to retain
high quality.

Interestingly the xbeglo was not that easy to do. I've come across this before, the file beginning
and ending are filsystem things. Yout can't cut down a file to a certain size so easily


As I was gettigng closer to using libmp3splt and EDL
I noticed that EDL was gettign reported back as long.

I mean, the raw EDL is quite clearly float. WHy do long? 

I mean, you really need an explanation for that.
