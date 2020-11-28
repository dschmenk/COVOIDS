# COVOIDS!

![covoids](https://github.com/dschmenk/COVOIDS/raw/main/covoids.png)

MS-DOS game featuring COVID virus as shoot-em up targets ala Asteroids

To run this game, copy to media connected to an MS-DOS computer. Anything from an original IBM PC to a 386 machine to a Pentium III. If you have a modern machine, you can install DOSBox (https://www.dosbox.com) and run the game under emulation.

Kill the COVID virus using the left, right, and up arrow keys to direct your ship and firing with the spacebar.

Watch the video: https://youtu.be/WPZTF6Pnx8M

COVOIDS! uses the GFXLIB developed for the Bresenham-Span project: https://github.com/dschmenk/Bresen-Span/tree/master/src/gfxlib

Since COVOIDS can run on everything from a 4.77 MHz 8088 with CGA on up, there are graphical setting to adjust the quality vs framerate options.

Command line switches to adjust the visual quality and performance: 

    -n disable dithering
    -m monochrome monitor
    -s disable background sound
    -d4 use EGA mode on VGA
    -d2 use CGA mode on EGA & VGA

The code is built with Borland C++ 3.1 on a Compaq 286 running MS-DOS 5.0. The BUILD.BAT file compiles and links everything, no makefile here!

Enjoy while you're (still) sheltering at home. Hopefully we'll all get through this in a timely manner
