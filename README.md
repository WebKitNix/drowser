Drowser
=======

A desktop browser using WebKitNix, with example implementations of its Platform API.

Pre-requisites
==============
The code uses some C++11 features like range-based for loops, closures and variadic templates,
so using GCC >= 4.7 will make your life easier.

Compiling
=========

Make sure you have Nix and all their libraries installed and pkg-config can find them:

1. Go to WebKitNix source directory and build it with --prefix. For example:

        $ build-webkit --nix --release --makeargs=-j8 --prefix=~/nixbuild

2. Go to WebKitNix output directory and install it:

        $ cd Release
        $ make install

3. Now setup your environment (where '~/webkitnix/WebKitBuild' is your WebKitNix output directory):

        $ export LD_LIBRARY_PATH=~/nixbuild/lib/:~/webkitnix/WebKitBuild/Dependencies/Root/lib64/
        $ export PKG_CONFIG_PATH=~/nixbuild/lib/pkgconfig/:~/webkitnix/WebKitBuild/Dependencies/Root/lib64/pkgconfig/

4. Make sure you can find WebKitNix on your pkg-config

        $ pkg-config --cflags --libs WebKitNix

5. Now go to Drowser source directory and do:

        $ mkdir build
        $ cd build && cmake ..
        $ make

6. And finally run:

        $ ./src/Browser/drowser

Troubleshooting
===============

Make sure you don't have LIBRARY_PATH set. Mixing it with pkg-config on CMake may cause linking errors.

Alternative build system: meique
================================

Drowser started as a pet project, and used other pet project as build system: meique
(http://www.meique.org). The meique.lua files are the equivalent of CMakeLists.txt. They are kept
for historical reasons and Hugo maintains them, so contributors don't need to worry about them.
