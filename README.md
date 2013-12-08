Made (or rather, converted) by JustJanek

Compiled using GCC on Ubuntu/Linux. 
Might work with MingwGCC or CygwinGCC on Windows but hasn't been tested thus far.
	
Might not be the prettiest but it works. The original python netboot code (the one I used to 
make this) is more detailed and has more functionality but if you only used it to upload 
games to the net dimm then this will suffice.

"What's the point?" you might ask. First off, it takes away the time necessary finding out how
python works, what version of python you need and what libraries you need to scour the internet over
to find. When I finally got my net dimm I was excited but it still took a couple of weeks finding out
how things worked and what to install. If you got this as binary you're good to go.
Secondly, if you're like me and want to expand on this netboot upload tool (e.g. make some automatic
upload system using a web interface) but want to use C rather than python you now can using this
maybe you will have to add some externally available function in this file to allow other .c files
to upload a game.

The usage is ./<binary name> <ip> <gamefile>
An example would be: ./netboot_upload_tool 192.168.0.123 ikaruga.bin

Compiling is as simple as opening the console and typing in 'make' if you have make and GCC installed.

Check out my (newly created blog) at for more information about this project or future projects: http://justjnk.blogspot.nl/
