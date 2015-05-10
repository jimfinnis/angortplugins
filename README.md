I've deliberately kept the build structure very simple - there's just a
separate build script in each directory, which builds the library into the
libs directory. This is to make it simple for you to devise a build script for
your own plugin libraries.

The install script copies any libraries in or below the libs
directory into the appropriate place (usr/local/share/angort, typically,
but this can be changed).

It is up to you to build which libraries you need. Typically the IO library
will be required. From this directory, 

    cd io
    ./build

Of course, Angort will need to be installed first, and any prerequisites
for the library. Then go back into the top directory and

    sudo install
