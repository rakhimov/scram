#!/usr/bin/env bash

set -e

git clean -fd

git rm -rf _images/ _sources/ _static/ genindex.html index.html \
  objects.inv search.html searchindex.js doc/ api/ README.*

[[ -d src ]] && rm -rf src
[[ -d tests ]] && rm -rf tests

mkdir api

cp -r build/publish/* .
cp -r build/html/* .
cp -r build/api/html/* api/

[[ -f publish.sh ]] && rm publish.sh

git add .
