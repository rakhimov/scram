#!/usr/bin/env bash

set -e

git clean -fd

git rm -rf _images/ _sources/ _static/ doc/ api/

cp -r build/html/* .

[[ -f publish.sh ]] && rm publish.sh

git add .
