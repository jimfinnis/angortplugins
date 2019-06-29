nvBDFlib
version 1.0 (9th July 2014)
(c) 2014 Giuseppe Gatta
----------

What is nvBDFlib?
------

BDFlib is a library to handle BDF fonts, such as the ones shipped 
with the X Window System, in an easy and quick manner.
This library does not just support loading the fonts, but also 
drawing text with them - one just needs to provide a callback
function to do the actual drawing, and little else.

The library was developed with simplicity in mind and thus does
not depend on anything except for a standard C library.

Functions
------
For a list of functions, look inside nvbdflib.h - everything is documented
in the Javadoc style. If you don't know what that is, don't worry,
most probably you will understand it anyway.

Example program
------
An example program is shipped with this distribution, called "bdfbanner",
an implementation of the well known "banner" program which uses 
nvBDFlib, demonstrating the library's features.
It does not depend on anything but the standard C library, and text
is rendered as ASCII art.
The program assumes that the displaying terminal is 80 columns wide.

term14.bdf is a BDF font you can test with, it was released by
Digital Equipment Corporation under the MIT license.

License
------

The library is licensed under a 3-clause BSD license; for further
information read LICENSE.txt.
The source code of the example program is placed in the Public Domain,
as such all copyright claims on it are waived.

Contact
------
This library was developed by Giuseppe Gatta (nextvolume).
The author can be contacted via the email address: tails92@gmail.com
