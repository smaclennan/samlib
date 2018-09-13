#!/usr/bin/python

from __future__ import print_function
import sys
from PIL import Image

if len(sys.argv) > 1:
    try:
        im = Image.open(sys.argv[1])
        width, height = im.size
        print(str(width) + "x" + str(height) + " " + im.format)
    except Exception,e:
        print(e, file=sys.stderr)
        sys.exit(1)
else:
    print("I need a file name", file=sys.stderr)
    sys.exit(1)
