#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/massgrid.png
ICON_DST=../../src/qt/res/icons/massgrid.ico
convert ${ICON_SRC} -resize 16x16 massgrid-16.png
convert ${ICON_SRC} -resize 32x32 massgrid-32.png
convert ${ICON_SRC} -resize 48x48 massgrid-48.png
convert massgrid-16.png massgrid-32.png massgrid-48.png ${ICON_DST}

