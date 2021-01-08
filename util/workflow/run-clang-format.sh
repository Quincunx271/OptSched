#!/usr/bin/env

shopt -s globstar
clang-format-9 -style=file -i "lib/**/*.[ch]pp include/**/*.h unittests/**/*.[ch]pp"
