# Mandelguts
### a mandelbrot set visualizer which tries to find interesting color patterns in the set's interior

Most visualizers for the Mandelbrot set put lots of colors around the outside of the set, but none inside.
For this one, I tried to find ways to color the insides, and make patterns and behaviors visible there.
I tried a few approaches, working toward ones which made a better looking image.
The basic idea is that points inside the set tend to fall onto looping attractors.
The primary color of the point is determined by the number of steps in the loop,
which generally increases as we go from the central mass toward outlying islands.
That color is then adjusted in luminosity according to how widely spread the points are.

The code is in C++, written many years ago for the old Watcom compiler when it was commercial.
(I have not included all the GUI clutter files it generated.)
The exe here is windows-32.
The UI is operated by keyboard or by a right-click menu, which shows the keystrokes.
Zooming in to a piece of the map can be done by drag-selecting a rectangle with the mouse.

I redid some of the code in Java just to compare speed... I'm not sure the Java version was ever working correctly.
In general this was never meant to be a slick polished public release — just some rough-and-ready code for personal experimentation.

No license, use it as you wish.
