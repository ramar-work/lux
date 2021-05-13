# app.hypno

This is an example app written in C with a few different routes.
It's mostly for testing purposes, but a real app can be written with this tooling.


## Files

app.so - The app in shared library form.

dylib.c - Tooling to test the created library.

main.c - The source code of the app.

static/ - Files that will be served directly.

vendor/ - Local copy of dependencies that aren't distributed with Autoconf yet.

Makefile - Recipes to create the shared library and dynamic library tester. 

