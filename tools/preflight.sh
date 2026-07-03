#!/bin/bash -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
! find . -type d \( -name pocs -o -name patches \) -print -quit | grep -q .
! find . -type f \( -iname '*poc*' -o -iname '*reference.patch' -o -name BUGS.md \) -print -quit | grep -q .
! grep -R -n -E 'curl|wget|git clone|apt-get|pip install|npm install' --exclude-dir=build --exclude=preflight.sh .
grep -q '^language: c++$' .clusterfuzzlite/project.yaml
printf 'preflight passed\n'
