#!/usr/bin/env bash

git clean -fd

git rm -rf _images/ _sources/ _static/ genindex.html index.html \
  objects.inv search.html searchindex.js doc/ api/

rm -rf ./src ./tests

mkdir api

cp -r build/html/* ./
cp -r build/api/html/* api/

git add .
