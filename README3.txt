
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

libmp3splt comments
-------------------
the primary struct is the state. That is the basis one upon which you build up.

There are various callback functions, where you define your function and then register it with a callback function which are "set_function" type things.

libmp3splt is also a rather high level API, it will do alot of things itself. You can give it an impossible long
time point for a split point and it will just insert the real end of the file without your needing to calculate it.

The main problem is that its timings are a bit different to the EDL and mplayer formats


chewaud.c and how legacy builds up.
-----------------------------------
WHen I started maniuplaitng wav files, I had use this little function of mine fszfind() whihc just finds the size of a file.
It's in quite a few pieces of code I have. I don;t know why I was so rpoud of it, the stat functiojn is able to do that and much more.
and it doesn't open the file either (clearly it must use the FS for that). Anyhow I had left it in, despite now using stat and
of course it used rewind() and was causing the input file to be re-read. SO that the FD wouldsteelle febfore the wavheader and write it into the first split ffile.
It was a devil to find, and I had to program a sine wav generator to find it! That'l learn ya!


