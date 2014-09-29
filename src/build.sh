#!/bin/bash

# useful build script for purpose of testing

f="-O -Wall -DCONFIGPATH=\"meta\" -fPIC"

e() { echo error: "$@" >&2; exit 1; }
d() { echo running: "$@" >&2; "$@" || e "$@"; }

[ -f meta ] || e no file meta

d gcc $f -o toolbox toolbox.c parser.c buffer.c foreign.c sha256sum.c
d ./toolbox h > tzplatform_variables.h
d ./toolbox c > hash.inc
d ./toolbox signup > signup.inc
d gcc $f -c *.c
d ld -shared --version-script=tzplatform_config.sym -o libtzplatform-shared.so buffer.o   foreign.o  heap.o  parser.o  scratch.o context.o  hashing.o  init.o  passwd.o  shared-api.o
d ar cr libtzplatform-static.a static-api.o isadmin.o
d gcc -o get tzplatform_get.o static-api.o -L. -ltzplatform-static -ltzplatform-shared


