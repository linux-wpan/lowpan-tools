#! /bin/sh
set -e
autoreconf --verbose --install
./configure "$@"

echo
echo "Now type 'make' to compile"

