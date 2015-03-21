#!/usr/bin/env bash

# Run Gource to create video from the Git history of a project.
# The script needs Gource and avconv.

gource -s 0.25 -a 0.5 -1280x720 -o - \
  | avconv -y -r 60 -f image2pipe -vcodec ppm -i - -vcodec libx264 \
  -preset ultrafast -pix_fmt yuv420p -crf 1 -threads 0 -bf 0 gource.mp4
