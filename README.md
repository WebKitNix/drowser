Drowser
=======

A demo browser for WebKitNix, using its C and Nix::Platform APIs

Compiling
=========

Drowser started as a pet project, so it uses other pet project as build
system, to compile it you will need to have meique (http://www.meique.org)
on your system, then:

    $ mkdir build
    $ cd build && meique ..

Make sure you have Nix and all their libraries installed and pkg-config can find them. The code
also uses some C++11 features like foreach loops, closures and variadic templates, so using GCC >= 4.7 will
make your life easier.

Not happy with the build system choice? Write the e.g. cmake files and keep supporting them.
