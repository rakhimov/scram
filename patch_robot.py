#!/usr/bin/env python
"""Patches an html file to disable access for robots."""

import sys

for html_file in sys.argv[1:]:
    html = open(html_file, 'r')
    contents = html.readlines()
    html.close()
    contents.insert(contents.index('<head>\n') + 1,
                    '<meta name="robots" content="noindex, nofollow"/>\n')
    html = open(html_file, 'w')
    html.write(''.join(contents))
    html.close()
