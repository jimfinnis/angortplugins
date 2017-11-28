# New build system
The plugins are built with a new CMake build system, where a single
CMakeLists.txt builds all the plugins. For each one, it will
check library dependencies (Curl, SDL, JACK etc).
Just do

    mkdir build
    cd build
    cmake ..
    make
    
You should end up with a load of *.angso* files. Running

    sudo make install
    
should then install them into '/usr/local/share/angort'.


# Old documentation
> The old build system still survives, as the various *build*,
> *buildall* and *reallybuildall* files. This describes it.
> **DO NOT USE** unless you really have to **OR** want to understand
> how to build Angort plugins (CMake can be a bit hard to read).


I've deliberately kept the build structure very simple - there's just a
separate build script in each directory, which builds the library into the
libs directory. This is to make it simple for you to devise a build script for
your own plugin libraries.

The install script copies any libraries in or below the libs
directory into the appropriate place (usr/local/share/angort, typically,
but this can be changed).

It is up to you to build which libraries you need, although
the **buildall** script will build the basic set: IO, regex, time, sys.
The others are for odd applications I need myself, and **reallybuildall**
will build those. They're here because they're fun and provide useful
example code.
So to build the standard set:

    ./buildall
    sudo ./install
    
The latter line will copy any **.angso** (Angort shared objects, i.e.
plugins) into /usr/local/share/angort.

To build a single library (e.g. SDL, which has quite a few dependencies -
see the build script in the SDL directory):

    cd sdl
    ./build

Of course, any prerequisites
for the library will need to be installed first.
Then go back into the top directory and

    sudo install


If you want to make your own plugin, a good place to start is
by looking at example_hello for a very simple example, and
example_complex for one which defines a type. From there,
io shows how to make an iterable type.
