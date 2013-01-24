Drowser
=======

A desktop browser using WebKitNix, with example implementations of its Platform API.


Compiling
=========

Make sure you have Nix and all their libraries installed and pkg-config can find them. The code
also uses some C++11 features like range-based for loops, closures and variadic templates, so using
GCC >= 4.7 will make your life easier.

    $ mkdir build
    $ cd build && cmake ..
    $ make


Alternative build system: meique
================================

Drowser started as a pet project, and used other pet project as build system: meique
(http://www.meique.org). The meique.lua files are the equivalent of CMakeLists.txt. They are kept
for historical reasons and Hugo maintains them, so contributors don't need to worry about them.
